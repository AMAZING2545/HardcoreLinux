# Moving Obsidian Core Linux to an independent repository

The current GitHub repository is still technically a fork of the original HardcoreLinux repository. GitHub does not automatically remove that relationship when the repository content is renamed.

Fork status does not propagate commits back into the original repository. Changes remain in the fork unless a pull request is opened against another repository.

To remove the fork relationship completely, create a new empty repository under your GitHub account and copy the project into it.

## Recommended migration

Create a new empty repository named something like `ObsidianCoreLinux` from GitHub. Do not initialize it with a README, license, or `.gitignore`.

Then locally:

~~~bash
git clone https://github.com/Yassine-Jemi01/HardcoreLinux.git ObsidianCoreLinux
cd ObsidianCoreLinux
git checkout distro-foundation
git switch -c main
git remote set-url origin https://github.com/Yassine-Jemi01/ObsidianCoreLinux.git
git push -u origin main
~~~

After verifying the new repository, set `main` as the default branch.

The new repository is a normal independent repository. The original HardcoreLinux repository receives nothing from it unless you deliberately interact with it.

## Preserving every ref

For a complete mirror including all branches and tags:

~~~bash
git clone --mirror https://github.com/Yassine-Jemi01/HardcoreLinux.git
cd HardcoreLinux.git
git push --mirror https://github.com/Yassine-Jemi01/ObsidianCoreLinux.git
~~~

Use the working-tree method above when you want the current distro branch to become the clean public `main` branch.