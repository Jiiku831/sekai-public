## Short instructions

Install [bazelisk](https://github.com/bazelbuild/bazelisk):

*  With go: `go install github.com/bazelbuild/bazelisk@latest`
*  With node: `npm install -g @bazel/bazelisk`

### Setting up your profile

Edit the files in `data/profile` to set up your profile. The card
list `cards.csv` can be exported from the Cards tab of [this
spreadsheet](https://docs.google.com/spreadsheets/d/1_pHFBUTILVJbBFymBY08pVbrLu6VmYfPDuQifQKD0dA/edit#gid=1249058281)
(File > Download > Comma Separated Values (.csv)).

### Build teams

For known events, run `./build_event_team.sh --event_id=XXX`. For custom
events, edit the event parameters in `build_event_team_main.cc` and then run
`./build_event_team.sh`.

## Important Notes

*  You may consider setting up a private repo to save your profile. See the
   `sample_repo/` directory for example and instructions.
*  World link support is not added. You can run it and it probably will give a
   correct result but it may be slow.
*  The default point calculation is based on playing random songs on expert
   difficulty and assumes that pub skill values scales with your skill value.
   This means the importance of skill value is slightly exaggerated. You can
   change these assumptions if you know how to. Note, these assumptions may
   change without notice.

## Non-JP players before bloom fes update

*  Change `kSubunitlessOffset` to `10` in `sekai/event_bonus.cc`.

Note: there may be slight differences in points gained due to the use of JP
songs for point calculation.

## Contributing

Development dependencies: clang-format, pre-commit and buildifier:

```
sudo apt install clang-format pipx golang
pipx install pre-commit
go install github.com/bazelbuild/buildtools/buildifier@latest
cd /path/to/sekai-public
pre-commit install
```

There is a incomplete wasm port of this program at
https://jiiku.pages.dev/sajii. Contact me if you want to complete it.

## Detailed Instructions

This program is written in C++. Knowledge of using the Linux command line and
C++ will be helpful. **The usual warning about using the command line applies.
Improper use can cause system corruption or data loss.** If you are not
confident, it's best to do these in a VM or on a computer you dont care much
about.

### Set up build environment

#### Windows/Linux

Note: You may need to change some commands based on the distribution you are using.

1.  (Windows only) Install WSL. You may choose any distribution. If you don't
    have a preference you can just use Ubuntu which the following instructions
    are based on.
2.  Open the command line.
3.  Install python3, git, go and zsh: `sudo apt install python3 git golang zsh`
4.  Install bazelisk: `go install github.com/bazelbuild/bazelisk@latest`

#### Mac OS

1.  Install Homebrew
2.  Open the command line.
3.  Install go: `brew install go`
4.  Install bazelisk: `go install github.com/bazelbuild/bazelisk@latest`

### Clone the repository

1.  Run `git clone https://github.com/Jiiku831/sekai-public.git`.
2.  Navigate to the repository: `cd sekai-public`.
3.  (optional) Set up your personal private repository by following the
    instructions in `sample_repo`. Navigate to that directory if doing so.

### Set up your profile

1.  Make a copy of [this spreadsheet](https://docs.google.com/spreadsheets/d/1_pHFBUTILVJbBFymBY08pVbrLu6VmYfPDuQifQKD0dA/edit#gid=1249058281)
    *  Tip: You can open the repo in your file explorer by running
       `explorer.exe .` (Windows WSL), `xdg-open .` (Ubuntu Linux) or `open .`
       (Mac OS).
2.  Check the owned column and fill in the MR and SL in the Cards tab.
3.  Download the file as a CSV (File > Download > Comma Separated Values (.csv))
    and save it as `data/profile/cards.csv`.
4.  Open `data/profile/profile.textproto` and fill in your area item levels and
    character ranks.

### Run the program

1.  Fill in any constraints you want to apply in `data/profile/constraints.textproto`.
2.  Find the event ID of the event you want to build your team for. You can
    find this on https://jiiku831.github.io/sekarun.html.
3.  Run `./build_event_teams.sh --event_id=XXX`.
4.  The result will be at `reports/event_team.html`.

#### Custom events

For events not yet released. You can build a team for a custom event.

1.  Open `build_event_team_main.cc`.
2.  Search for the text `SimpleEventBonus`.
3.  Change the bonus attribute and characters.
4.  Run `./build_event_teams.sh`.
5.  The result will be at `reports/event_team.html`.
