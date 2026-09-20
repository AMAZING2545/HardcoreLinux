# Independent repository migration

This helper moves the current distribution line into a new independent GitHub repository.

Run from a clone of the project:

NEW_REMOTE=https://github.com/<user>/<new-repo>.git sh tools/migrate-independent.sh

The helper publishes the current distro-foundation line as main in the new repository.

The destination repository must be created as an empty repository first. Do not initialize it with a README, license, or .gitignore.