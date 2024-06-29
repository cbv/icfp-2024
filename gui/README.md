GUI
===

Status: does a few things

Threed Editor
-------------

```
Left-click-drag selects

C-x: cut (spreadsheet style --- the cut stuff doesn't go away until you paste)
C-c: copy
C-v: paste

Hover and type digits or SAB or operators to put stuff in cells
Delete or right-click or '.' to delete

'Copy Program' button to put the text of the program in the clipboard
'Expand' button to make the field bigger
```
Building
--------

Assuming you have nodejs installed,

```shell
$ make
```

should spin up a local http server on port 8000.

Commandline Tools
-----------------

- `scripts/send`

Sends stdin to their servers as a string literal, and expects a string literal from their server, and prints it on stdout.

Development
-----------

Other make targets are:

```shell
make watch # build js whenever ts source changes
make build # build once
make check # typecheck js whenever ts source changes
make test  # run tests
```
