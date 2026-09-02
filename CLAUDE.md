# Working in tlib

## Read the coordination channels first

Three Claude sessions work around this codebase, and **none is notified of the
others' writes** — a message once sat unread for four days because nothing
prompted a read.

| session | lives in | owns |
| :--- | :--- | :--- |
| sources | `faust`, `signals`, the `tlib/` sources | the code |
| documentation (this one) | `tlib/*.md`, `tour-examples.cpp` | the guide and its method |
| article JFLA 2027 | `FAUST-JFLA2027/` | `document.md` and its material |

Each session writes its own journal per correspondent, and reads the others'.
So, **at the start of any task touching tlib, and before answering "any
news?":**

```
tail -80 ~/Documents/Install/faust-migration/DIALOG.md   # sources → us
tail -60 ~/Documents/Install/FAUST-JFLA2027/DIALOG-TLIB.md  # article → us
tail -40 DIALOG.md                                       # what we last told sources
tail -40 DIALOG-JFLA.md                                  # what we last told the article
```

We write replies at the **end** of `tlib/DIALOG.md` (to the sources session) or
`tlib/DIALOG-JFLA.md` (to the article session), under a dated header
`## AAAA-MM-JJ — tlib`, never inside an older entry. Each item carries a status:
**INFO** (nothing expected), **ACTION** (something is expected of the other
side), **OPEN** (finding not yet closed), **CLOSED** (with what closed it). A
finding is closed when *every copy is fixed, built and tested* — not when it has
been reported. Both journals are gitignored: live scratch, not project
artifacts.

## Territory

Sources are the sources session's, `FAUST-JFLA2027/` is the article session's,
documentation is this one's.

| | |
| :--- | :--- |
| theirs (sources) | `tlib/*.hh`, `tlib/*.cpp`, and everything under `faust/`, `signals/`, `faust-migration/` |
| theirs (article) | everything under `FAUST-JFLA2027/` |
| ours | `tlib/*.md`, `tour-examples.cpp`, and its `CMakeLists.txt` target |

Need a source change? Write it in `DIALOG.md` as an **ACTION**; they apply it.
A sentence of the article that looks wrong? `DIALOG-JFLA.md`, **ACTION**; they
decide. The one exception, agreed with the sources side: a **literal port** of a
fix they have already validated and asked to be carried over — copy it without
rewriting, and report any line where you diverge.

**The article quotes the guide.** When it does, it quotes a sha: an excerpt is
only as true as the stamp of the chapter it came from. Before answering any
question from the article session about a code excerpt, re-derive the anchors
against the current HEAD rather than trusting the stamp.

## Three vendored copies

`tlib/` is duplicated into `faust/compiler/tlib/` and `signals/tlib/`. They must
stay byte-identical:

```
diff -rq tlib ~/Documents/Install/faust/compiler/tlib
diff -rq tlib ~/Documents/Install/signals/tlib
```

## Build and test

```
cmake --build build && (cd build && ctest)
```

Four tests must pass. `tlib-tour-examples` is the guided tour's claims made
executable — if it fails, a documented behaviour has changed.

## The documents

- `A-GUIDED-TOUR-OF-TLIB.md` — the guided tour, thirteen concepts in dependency
  order. Each ends with the commit its line references were checked against;
  re-verification is a diff against that sha, not a re-read.
- `CONCEPT-TOUR-AUTHORING.md` — how the tour is written and verified. **Read it
  before editing the tour.** It carries the rules that cost the most to learn:
  the code wins over the comment, beware the *faithful lie* (prose false
  *because* faithful to a stale comment — search the call sites), weaken
  absolutes, and the code asserts while the tour explains.
- `REWRITE-SPEC.md`, `SIGNATURE-SPEC.md` — specifications, in French.
- Rendered by [markpage](https://markpage.org); follow
  `~/Documents/Install/markpage/AI-AUTHORING.md` for its constructs.

After editing a document, check it mechanically: every `file:line` reference in
range, no inline `$…$` split across a line break, `::: toc+` entries matching
the headings, containers and fences balanced.
