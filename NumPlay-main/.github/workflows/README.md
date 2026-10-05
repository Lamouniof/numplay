# NumTycoon GitHub Actions build

The `build-numtycoon.yml` workflow compiles `games/numtycoon` on GitHub's Ubuntu runner and publishes `output/numtycoon.nwa` as a downloadable artifact.

## How to use it

1. Put the project in a GitHub repository.
2. Push the project to GitHub.
3. Open **Actions** → **Build NumTycoon (.nwa)**.
4. Select **Run workflow** if you want to launch a build manually.
5. Open the completed workflow run.
6. Download the **NumTycoon-nwa** artifact.

The workflow installs the ARM GNU toolchain and uses `nwlink@1.0.0`, matching the NumTycoon Makefile.
