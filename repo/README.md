# Native package repository

The legacy repository keeps the original tar archives for compatibility.

Native yspm packages use this layout:

~~~text
repo-yspm/
└── releases/
    └── 1/
        ├── index.json
        └── packages/
            └── *.yspkg
~~~

The release workflow can generate this repository from the existing legacy packages.

A local migration looks like:

~~~bash
python3 tools/migrate-legacy-repo.py \
  --input repo \
  --output repo-yspm/releases/1 \
  --base-url https://example.org/hardcorelinux/releases/1/packages \
  --yspm /tmp/yspm
~~~

The native repository follows yspm's stable-release and ABI model.
