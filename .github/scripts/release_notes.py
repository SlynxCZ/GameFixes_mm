#!/usr/bin/env python3
"""
Release notes for a tag, in the style GitHub generates them -- but from the
commits since the previous tag, since most changes land on main directly
rather than through pull requests:

    ## What's Changed in v1.0.36
    * Fix a crash in TakeDamage by @someone in #12 (abc1234)
    ...
    ## New Contributors
    * @someone made their first contribution in #12

Writes the notes to the GitHub release (--update-release) and posts them to a
Discord webhook ($RELEASE_WEBHOOK), the way CounterStrikeSharp announces its
releases. Needs GITHUB_TOKEN and GITHUB_REPOSITORY (both set in Actions).
"""

import argparse
import json
import os
import re
import subprocess
import sys
import urllib.request

API = 'https://api.github.com'
SKIP = re.compile(r'^(Merge (branch|pull request)|Bump version|chore\(release\)|release:)', re.I)


def git(*args):
    return subprocess.run(['git'] + list(args), capture_output=True, text=True, check=True).stdout.strip()


def gh(path, method='GET', body=None):
    req = urllib.request.Request(API + path, method=method,
                                 data=json.dumps(body).encode() if body is not None else None)
    req.add_header('Authorization', 'Bearer ' + os.environ['GITHUB_TOKEN'])
    req.add_header('Accept', 'application/vnd.github+json')
    req.add_header('User-Agent', 'source2toolkit-release-notes')
    if body is not None:
        req.add_header('Content-Type', 'application/json')
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.loads(r.read() or b'null')


def previous_tag(tag):
    try:
        return git('describe', '--tags', '--abbrev=0', '--match', 'v*', tag + '^')
    except subprocess.CalledProcessError:
        return None


def build_notes(repo, tag, prev, project):
    url = 'https://github.com/' + repo
    rng = '%s..%s' % (prev, tag) if prev else tag
    log = git('log', '--no-merges', '--format=%H%x1f%s', rng)
    lines, firsts = [], []
    seen_authors = set()
    for row in filter(None, log.splitlines()):
        sha, subject = row.split('\x1f', 1)
        if SKIP.match(subject) or subject.strip().lower() == 'pushbuild':
            continue
        subject = subject.strip()
        info = gh('/repos/%s/commits/%s' % (repo, sha))
        login = (info.get('author') or {}).get('login')
        pulls = gh('/repos/%s/commits/%s/pulls' % (repo, sha)) or []
        pr = next((p for p in pulls if p.get('merged_at') and p['base']['ref'] == 'main'), None)

        text = '* ' + subject
        if login:
            text += ' by [@%s](https://github.com/%s)' % (login, login)
        if pr:
            text += ' in [#%d](%s)' % (pr['number'], pr['html_url'])
        text += ' ([%s](%s/commit/%s))' % (sha[:7], url, sha)
        lines.append(text)

        # first contribution: no commit of theirs before the previous tag
        if login and login not in seen_authors:
            seen_authors.add(login)
            if prev and pr:
                before = gh('/repos/%s/commits?author=%s&sha=%s&per_page=1' % (repo, login, prev)) or []
                if not before:
                    firsts.append('* [@%s](https://github.com/%s) made their first contribution in [#%d](%s)'
                                  % (login, login, pr['number'], pr['html_url']))

    out = ["## What's Changed in %s" % tag]
    out += lines or ['* Maintenance release.']
    if firsts:
        out += ['', '## New Contributors'] + firsts
    if prev:
        out += ['', '**Full Changelog**: [%s...%s](%s/compare/%s...%s)' % (prev, tag, url, prev, tag)]
    return '\n'.join(out)


def discord_text(project, repo, tag, notes):
    release_url = 'https://github.com/%s/releases/tag/%s' % (repo, tag)
    head = 'A new release of %s has been tagged [%s](<%s>)\n\n' % (project, tag, release_url)
    text = head + notes
    if len(text) > 2000:  # Discord's message limit
        more = '\n\n… and more — [full release notes](<%s>)' % release_url
        cut = notes[:2000 - len(head) - len(more)]
        cut = cut[:cut.rfind('\n')] if '\n' in cut else cut
        text = head + cut + more
    # Keep embeds out of the post: <link> suppresses the preview.
    return re.sub(r'\]\((https://[^)>]+)\)', r'](<\1>)', text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('tag')
    ap.add_argument('--project', default='Source2Toolkit')
    ap.add_argument('--update-release', action='store_true', help='write the notes into the GitHub release')
    ap.add_argument('--discord', default=os.environ.get('RELEASE_WEBHOOK', ''))
    args = ap.parse_args()

    repo = os.environ['GITHUB_REPOSITORY']
    prev = previous_tag(args.tag)
    notes = build_notes(repo, args.tag, prev, args.project)
    print(notes)

    if args.update_release:
        rel = gh('/repos/%s/releases/tags/%s' % (repo, args.tag))
        gh('/repos/%s/releases/%d' % (repo, rel['id']), 'PATCH', {'body': notes})
        print('Release notes written.')

    if args.discord:
        body = json.dumps({'content': discord_text(args.project, repo, args.tag, notes),
                           'username': args.project, 'allowed_mentions': {'parse': []}}).encode()
        req = urllib.request.Request(args.discord, data=body, headers={
            'Content-Type': 'application/json', 'User-Agent': 'source2toolkit-release-notes'})
        try:
            urllib.request.urlopen(req, timeout=30).read()
            print('Posted to Discord.')
        except Exception as e:  # the release is out either way
            print('Discord webhook failed: %s' % e)
    return 0


if __name__ == '__main__':
    sys.exit(main())
