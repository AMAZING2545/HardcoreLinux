#!/bin/sh
set -eu

: "${NEW_REMOTE:?usage: NEW_REMOTE=https://github.com/<user>/<new-repo>.git sh tools/migrate-independent.sh}"

git fetch origin --prune
git switch distro-foundation
git branch -f main distro-foundation
git remote set-url origin "$NEW_REMOTE"
git push -u origin main

printf '%s\n' 'Independent repository published.'
printf '%s\n' 'Set main as the default branch in GitHub before deleting the old fork.'