# Writing a guided tour of a codebase

A method for producing the kind of document that explains *what a
library is made of and why it has that shape* — as opposed to a README
(what it does), a specification (what it must do), or API docs (how to
call it). The output is a single document that walks the concepts of a
system in dependency order, each concept explained four times over at
increasing precision.

Written for an AI agent doing the drafting, with a human reviewing
concept by concept. `A-GUIDED-TOUR-OF-TLIB.md` in this repository is the
worked example.

---

## When this method fits

| Situation | Use this method? |
| :--- | :--- |
| A library whose design is not obvious from its API | yes — this is the case it is for |
| Onboarding a competent programmer into unfamiliar theory | yes |
| A system whose parts only make sense together | yes |
| "How do I call function X" | no — API docs |
| "What must this component guarantee" | no — a specification |
| A change log, a tutorial, a quickstart | no |

The test: if the honest answer to "why is it built this way?" takes
three paragraphs and a reference, this method pays. If it takes one
sentence, write the sentence.

---

## Agree on the method before writing

Do not draft the whole document and then negotiate. Propose the section
template, the concept order and the audience; get agreement; write **one
concept**; get feedback on length, depth and tone; only then continue.
The first concept is the contract for the remaining ones.

Three questions must be answered before drafting:

- **Audience.** Not "developers". Which knowledge is assumed and which
  must be built? "A C++ programmer who knows compilers by practice but
  not the theory" determines every paragraph of the informal sections.
- **Language.** Not automatically the language of the conversation, and
  not always obvious from the repository: mixed-language projects are
  common (README in English, specifications in French). Decide from the
  intended readership and from what the document sits next to.
- **Depth per concept**, expressed as a target length for the first one.

---

## Shape of the document

```tree
Title + one-paragraph statement of purpose
  ::: toc+          — the plan and the table of contents in one
  How to read       — the section template, the reading paths, conventions
  Concept 1..N      — six fixed sections each
  (optional) Bibliography, written last
```

**Order concepts by dependency, not by history.** Not the order the code
was written in, not the order of the source tree — the order in which
each concept needs the previous ones to be stated. Announce the ordering
principle, then say plainly where it is imperfect: a first concept
usually has to name the carrier type it manipulates before that type has
its own chapter. Claiming a strict layering that the text then violates
costs more credibility than admitting the loop.

**Write the `::: toc+` plan first.** It doubles as the spec you check the
content against, and unwritten entries render struck through, so the
table of contents is also a progress indicator.

---

## The six sections

Every concept gets the same six headings in the same order, so a reader
can skip a level uniformly across the document.

| Section | Answers |
| :--- | :--- |
| **The idea** | what it is, informally, entered through what the reader already does |
| **Its role in X** | why the system needs it; what breaks without it |
| **More precisely** | the formal statement, once intuition is in place |
| **In the code** | how it is realised, with pointers to the source |
| **Invariants and non-goals** | what must stay true; what it deliberately refuses to do |
| **Origins** | where the idea comes from; what to read |

Fixed headings, **variable weight**. A minor concept may have a
`More precisely` of two lines. Do not pad a section to fill the
template; do not drop the heading either — its presence is what makes
the document skimmable.

### The idea

Enter through the reader's existing practice, never through the theory.
Describe a task they have done by hand, let them notice the pattern,
*then* name it. The names of the concepts should arrive as labels for
something already recognised.

> Write those four passes by hand and you notice they are the same
> program four times. […] Universal algebra gives names to the three
> things at play here.

End on the observation that reframes everything — the sentence the
reader should still have in mind three chapters later. Every concept has
one, but it is usually the last thing to stabilise: write a frank
placeholder in the first draft rather than stalling on it, and come back
once the formal and code sections have taught you what the concept
really turns on.

Use a concrete, small, *runnable-looking* example. Prefer one that also
demonstrates a magnitude ("31 nodes for a term with a billion leaves")
over one that merely illustrates a shape.

### Its role in X

The section that makes a tour out of a list. Two jobs:

1. **Locate the boundary.** What the system provides, and what it
   deliberately leaves to its clients. Be precise here; an overstated
   boundary ("X provides the syntax algebra") is the most likely
   technical error in the whole document.
2. **Justify what comes next as a demand.** Each later concept should
   appear here as the answer to a requirement this one creates, with a
   forward reference. This turns the remaining chapters into promises
   rather than a table of contents.

A one-sentence summary in bold at the end of the section, for the reader
who keeps nothing else.

### More precisely

Concise, and only what the rest of the document will actually use.

- **Every formula gets a prose paraphrase**, immediately after, as a
  dash-introduced clause or sentence. No exceptions. The paraphrase
  should say what the formula *means for the reader*, and may point at
  what the formula does **not** contain — often the real content.
- **Economise notation.** Each new symbol is a cost. If a notation will
  be needed later for something more important, do not spend it now
  (reserve $μ$ for recursion inside terms rather than for the recursive
  type definition, and say so).
- **Prefer the standard notation that avoids ambiguity.** $|𝒜|$ for a
  carrier is standard but collides with cardinality; naming the carrier
  $A$ is equally standard and reads better.
- **Split the consequences.** Do not state a theorem and move on; state
  separately each consequence the programmer will rely on, and what it
  licenses. "Uniqueness is what makes memoisation forced rather than
  optional" is worth more than the theorem.
- Say explicitly which cases the statement does *not* cover, and which
  chapter handles them.

### In the code

**Point, do not paraphrase.** Links to `file.ext:line`, API signatures,
and short excerpts only where the excerpt *is* the argument. Code copied
into prose goes stale at the first refactoring; a line reference degrades
gracefully.

Three things worth doing here:

- Reduce the mechanism to its irreducible core — "everything else is
  detail around those seven lines" — and show those.
- **Explain what looks superstitious.** A line that seems arbitrary
  (`fData.f = 0.0` before storing a narrower field) almost always
  encodes an invariant. Finding and stating it is the highest-value
  paragraph in the section.
- Quote real measurements from the code or from a run, and attribute
  them ("about 72% of trees never receive a property"). Never round a
  measured claim into a stronger general one.

### Invariants and non-goals

Bold lead-in per item, one short paragraph each. Two kinds of content:

- **Invariants**: what must stay true for the rest of the system to
  work.
- **Non-goals**: what the concept refuses to do, especially where a
  reader would assume otherwise.

Write the non-goals against the **most likely misreading** of the
sections above. If `More precisely` presented terms as an algebra, the
reader will assume the type enforces well-formedness — say that it does
not. This section is where a careful document earns its trust.

Note deliberate limitations honestly, including platform-specific
behaviour and stale comments in the source that would mislead a reader
of the code.

### Origins

Short: a paragraph or three. One line of history, then one to three
references. Prefer:

- the paper that **introduced** the idea, even if unfashionable;
- the paper that best **demonstrates the payoff** (BDDs for
  hash-consing + memoisation), which is often more persuasive to an
  engineer than the founding paper;
- one **practical modern** reference if the founding papers are hard
  going.

Verify attributions. Do not credit a term to a paper that does not use
it; separate what a founding paper actually contains from what its
successors added.

---

## Conventions that carry the document

### Forward references

Unavoidable in a dependency-ordered text. Handle them with a **footnote
at first mention**, containing (a) a thumbnail definition sufficient to
follow the current argument, and (b) the section that develops it.

```markdown
… terms in a real compiler are recursive[^rec], which the definitions
above do not cover at all.

[^rec]: A **recursive term** is one that refers to itself, as in
$x = 1 + x$ — a finite piece of syntax denoting an infinite tree. […]
Developed in §8 (the terms) and §10 (their attributes).
```

State the convention in *How to read*, so the device is legible. It has
a pleasant side effect: it makes the reader curious about chapters they
have not reached.

The footnote list is also a vocabulary audit. Any term used without a
footnote and without an inline gloss is debt — a `Tree` mentioned in
chapter 1 and defined in chapter 3 needs one.

Keep each footnote to a few sentences. If they grow past that
systematically, the concept order is probably wrong.

### Cross-references

Use section numbers (`§5`) rather than titles, and put them where a
reader would ask the question, not where the topic is finished.

### Markup

Rendered by markpage — see `AI-AUTHORING.md` for the full set. What this
genre uses:

| Need | Construct |
| :--- | :--- |
| Plan + table of contents | `::: toc+`, written first |
| Inline mathematics | `$ … $` — not backticks |
| Display formula | ` ```math ` or `$$ … $$` |
| Syntax tree, term structure | ` ```tree svg ` — an *indented outline*, one node per line |
| Filesystem or module layout | ` ```tree ` |
| Algorithm in pseudocode | ` ```algorithm ` |
| Real code | ` ```cpp ` etc., short excerpts only |
| Forward reference, term definition | footnote `[^name]` |

Reserve backticks for genuine code: identifiers, files, types. A
mathematical symbol in backticks reads as an implementation detail.

A generic Markdown linter will complain about repeated headings
(`MD024`) — that is the template working as intended.

---

## Verification discipline

This is what separates a guided tour from a plausible essay. **Writing
this kind of document is a code review**, and it will find defects.
Budget for that.

The rules below are diagonal braces. A scaffold of posts and ledgers has
no weak member and deforms anyway, because nothing resists shear — and
the defects this method finds have that shape: code right, comment right
when written, prose faithful to the comment, whole thing drifted. Asking
*which member is at fault* is the wrong question. Hence bracing between
*different* members (a brace within one post does nothing), few of them,
each looking like dead weight until something pushes sideways — and none
of it transmitting anything unless fastened: a channel nobody is told to
read is a diagonal resting against the posts.

**Check every claim about the code against the code**, with a line
reference, at the moment of writing it. Not "the API is roughly"; open
the file.

**An artifact that cannot express an absence must be audited against the
instrument that produced it.** A table of findings says which claims
failed; it has no cell for a claim that was never examined, and the two
look alike from the outside. So when a listing, a report or a results
table is your evidence, do not read it on its own — read the selection
rule that filled it, against the rows it produced, and ask what the rule
could never have let in. The defect that survives every reading of the
output is the one that is not in the output.

**When a comment and the code disagree, the code wins** — and the
comment is a finding to report, not something to quietly repeat or
quietly ignore.

**Beware the *faithful lie*: prose can be false precisely because it is
faithful to its source.** This is the failure mode the two rules above
do not catch, because there is no disagreement to detect — the comment
and the code say compatible things, and the missing fact lies outside
both. Two shapes recur:

- *A comment names a consumer, or a share, that has since vanished.* "A
  fast slot for one hot property — in this compiler, the propagation
  memo, about 20% of property traffic." Accurate when written; the
  consumer had since moved out, and the field had no user at all. The
  code cannot say so: a field does not know whether anyone reads it.
- *A comment states a semantics the code expresses only indirectly.*
  `dnfLess(c1, c2)` was documented as `c1 ⟹ c2`; the body reads
  `c1 == dnfOr(c1, c2)`, which says the opposite. Reading the body is
  not enough either, unless you stop to work out what the expression
  *means*.
- *A comment survives a redesign of its own file.* The kind bits of a
  node were opened to consumers through a registered callback, then
  reworked the same day into data on a symbol; two lines of the header
  still described the callback, six lines above a paragraph describing
  the data. The file contradicted itself, and both halves had been
  written in good faith within hours.

Two habits catch both. **Search for the call sites** whenever a claim
names a user, a frequency or a share — grep the whole dependency chain,
not the file you happen to be in. And **let the tests arbitrate
semantics**: the code says what a function does, the test says what it
is *for*. Where a test and a comment disagree, the test is the better
witness, and the disagreement is a finding.

**Look for them where the code is freshest.** Intuition says stale
comments accumulate in old files. The four found while writing this
repository's tour say the opposite: each sat beside a recent change, one
of them the same day. A comment goes stale when someone corrects the
code and does not re-read what surrounds it — so the highest-yield place
to look is the neighbourhood of the last commit, not the oldest file.
When reviewing a diff, read the **unchanged** lines around it.

This axis is also the argument against writing a tour with only the
library in view. The claims most likely to be wrong are the ones about
what the code means to those who call it, which is exactly what a
narrow window hides — see *Route the findings* below.

**Measure rather than assume.** Sizes, ratios, complexities: run it. A
comment claiming `sizeof` is 112 does not make it 112.

**Weaken absolutes.** "Forces" is usually "makes cheap". "Is
exponential" is usually "is exponential in the worst case". "Enforced by
the type system" is often "enforced by the API as it is meant to be
used". Each softening is small; together they are the difference between
a document that survives scrutiny and one that does not.

**Distinguish what a mechanism guarantees from how it is spelled.** A
comparison may be semantically right while its implementation relies on
an unwarranted assumption; say both.

**Cross-read.** Independent passes over the same lines, motivated by
different questions, find different defects. Trying to explain *why* a
line exists finds different bugs than reviewing it for correctness. Use
a second reviewer (human or model) on the drafted prose, and verify its
claims too — a reviewer's confident correction can itself be wrong.

**Fix the code, do not paper over it in the prose.** If the document has
to describe a defect to stay honest, that is the signal to repair the
source. The natural temptation is the opposite: a well-turned paragraph
about a wart is satisfying to write, and it leaves the wart in place.

---

## Keeping it true

Verification at drafting time is only half the problem. A tour full of
`file.ext:line` references and measured figures starts rotting at the
next refactoring, and it rots **silently** — nothing fails, the document
simply becomes fiction. Plan for that from the first concept.

**Make the examples executable, as a build target.** The method above
asks for examples that look runnable; that recommendation is a trap
unless they *are* run. Put them in a compiled file (`tour-examples.cpp`)
wired into the test runner, and quote from it by line reference. An
example that is only prose will drift, and — the failure mode to fear —
it will drift while the test suite stays green, because nothing was
watching it. A demo or benchmark left out of `ctest` can assert a false
claim for weeks without anyone noticing; a tour is exactly as fragile.

**Stamp each concept with what it was verified against.** A line per
chapter — *code references verified at `<sha>`* — tells a reader how much
history separates the text from the current source, and turns
re-verification into a targeted diff instead of a full re-read.

**Move a stamp only after re-verifying, and never realign a reference by
arithmetic.** These are one rule seen from two ends, and this document was
written before its author had broken it. Re-verifying a chapter, one finds
references that were already wrong *at the sha the stamp claimed* — because
they had been "updated" by adding the size of a diff instead of by searching
for their text. Insertions are never uniform: one file had grown by one line
near its top, eleven a third of the way down, thirty-two lower still, so no
single offset was right for any two anchors. And a stamp that was advanced
without a real check is worse than no stamp at all, because its whole function
is to *extinguish doubt* — it tells the next reader not to look.

So: **re-derive every anchor by searching for the text it names**, and make the
check emit a readable trace — file, line, and the content of that line —
because nothing else distinguishes "verified" from "recomputed". A one-line
script that prints every reference next to the source line it lands on turns a
tedious act of faith into ten seconds of reading.

And the trace is not a formality, because the *repair* has its own failure mode.
Applying a hundred corrections as a batch of textual substitutions looks like
re-derivation and is not: a rule rewriting `286` to `287` leaves a `287` that
the next rule, `287` to `290`, immediately consumes. Three anchors were
silently merged onto their neighbours that way, and only the line-by-line trace
caught them before the stamp was moved. Batch substitution is arithmetic
wearing a costume. **What is verified is what the trace shows each reference
landing on** — not the map you applied to get there.

**And do not count a proxy for the claim.** The third variant of the same
mistake, and the hardest to see, because its output looks exactly like a
measurement. Asked whether four messages carried a request, I grepped for a
named recipient in the body and published a four-row table: four out of four.
The criterion was not "names someone", it was "expects an answer", and one of
the four named a colleague only to report to them. A second proxy, counting
question marks, gave one out of four — requests get written as imperatives.
Two cheap predicates, two wrong counts; only reading the four bodies gave the
right one.

**And do not report the shape of your instrument as a fact about the world.**
The fourth, and the one that catches people who have learned the other three.
Asked which schedulers had callers, I ran two greps: the first counted
occurrences of all fourteen names, the second printed call sites for a
hand-written pattern that held only seven. I published the second's silence as
an absence, with the first's non-zero counts still on the screen. The same day,
on a different subject, a colleague reported "three untracked files" from a
command that ended in `head -3`, and wrote the diagnosis better than I had:
*I passed off the shape of my own pipeline as a fact about the disk.* A
truncated listing and a narrowed pattern are the same mistake — the tool's
limit read as the world's.

The four share a shape: something adjacent and cheap is substituted for the
thing claimed, and **the substitution leaves no trace in the output**. A shifted
line number looks like a checked one, a paraphrase looks like a reading, a
proxy count looks like a count, and a truncated list looks like a short one. So
the question to ask of your own verification is not "did I check?" but *"is what
I measured the thing I am asserting?"* — and when the two differ by a step of
reasoning, however short, the step is where the error will be.

A practical corollary, cheap enough to adopt without discipline: **when a check
returns nothing, be suspicious of the check before you are satisfied with the
result.** An empty answer is the one an instrument gives when it is pointed
slightly away from the question, and it is also the one that feels like good
news.

A fifth case escapes even that corollary, because there the check *could
not* have answered. Coordinating with other sessions over a git-backed
message board, I ran the guard that detects a message published while I
was drafting: compare the tip I had read against the tip now. But I
recomputed "the tip I had read" inside the publishing step, after
re-fetching — so the guard compared a value to itself. A colleague's
message had indeed arrived in that interval; it entered my baseline
unread, and the guard reported nothing, as it would have on any input
whatever. The four variants measure the wrong thing. This one measures
nothing, and prints the same reassuring emptiness.

What sets it apart is the cost. A mismeasurement yields a false
sentence, and a false sentence can be contradicted later. This yielded a
missing input, and nothing downstream knew it was missing: the board's
reading cursor restarts from the agent's own last message, which was
committed thirteen seconds after the one I had skipped, so the skipped
message fell behind the cursor for good. **A silent check does not delay
the discovery of what it missed; it can remove the occasion to discover
it.** A guard whose two sides come from the same computation is not a
weak guard — it is not a guard.

That case has since recurred twice in a day, and the three together name
the cause, which is not haste.

```sh
[ "$a" \> "$b" ]          # this shell refuses it: one error per line, then carries on
git show "$t:readme.md"   # zsh reads `:r` as an expansion modifier, not two characters
```

The first printed an error per item, carried on, and left a neat table
whose verdict column had tested nothing. The second asked git for a
revision that did not exist — and a `2>/dev/null` appended to keep the
output tidy turned that fatal error into an empty result, four aligned
lines with no trace of the failure.
Both times the check had been written to *read well*: quiet, uninterrupted,
aligned. **The presentation of a check can eat its verdict**, and the
tidier the output, the better it hides that nothing was measured.

The remedy is the negative control that follows, and it is worth
resisting the tempting shorter version. *Never redirect `stderr` inside a verification*
is memorable, true, and covers only the second of these two: in the first,
nothing was suppressed at all — the errors were on screen, one per line,
and eighteen tidy rows came out looking complete beside them. A rule about
the error channel has nothing to say there, while a known-negative input
catches both. Keep the redirection sentence as an example of the rule and
not as the rule, since it is advice about one shell and the control is
about instruments. The same goes for a check that prints per item and
exits zero: it leaves the verdict to a reader who will read the shape of
the output rather than its status.

The corollary above says to doubt without saying how to end the doubt,
and the answer is the positive control of any laboratory bench: **before believing
a check written for the occasion, give it an input whose negative answer
you already know, and confirm it says no.** A check that cannot say no
cannot say anything. It costs seconds and it catches the whole family —
an order comparison fed a deliberately out-of-order pair must report it; a
search fed a case where the target certainly exists must find it; a
staleness guard fed a baseline two revisions old must fire. One-off checks
are where this matters, because a tool's check has run a thousand times
and its silence has a track record, while a check written once, read once
by eye and discarded has never been seen failing at all.

And notice where the control is most needed. A check run over material
that happens to contain faults is witnessed for free — it prints its
failures while working, and so demonstrates it can say no at no cost to
you. A check run over clean material is not witnessed at all: same code,
same care, no evidence, because the corpus supplied none. So the
unwitnessed check is systematically the one whose result is good news,
and good news is what one is least inclined to re-examine.

None of this shows a check is *right*, only that it is not inert. The one
rung above that is cheap and old: **emit two derived quantities that must
agree — a count beside the list it counts, a total beside its parts — and
publish them together.** With one condition, which is the guard's own
condition a storey up: the two must be **derived independently**. Two
copies of one computation always agree, and their agreement teaches
nothing — a witness compared with itself is no more a witness than a
guard compared with itself is a guard. A correct instrument makes them consistent; a
wrong one now has to be wrong twice in the same direction. It is what
catches the check that runs, discriminates, and is still wrong: a
colleague's audit printed "31 candidates, 31 reachable" above a list of
five that were not, and 31 + 5 exceeding 31 is what gave it away — not a
closer reading, but an output built so it could contradict itself. It is
also the only one of these habits that keeps working after you are gone:
a control serves whoever ran it, a redundant output serves every later
reader.

There is a second instrument, and the two catch different things. A
colleague re-deriving the same question by another road will not find a
broken instrument — it finds a working instrument answering a question
badly posed. A line reference of mine was contradicted that way, and both
numbers were right: the file had moved between the two readings, and no
negative control would ever have revealed it. **The control tests the
instrument; the second reader tests the question.**


The five have a constructive converse, and it is the more useful half.
Sooner or later the budget will not carry the whole check, and you will
cut something on purpose. Two reductions are then available and they are
not equivalent: cut the *field* — check some of the claims — or cut the
*grid* — check all of them on a sample of the material. The first is
invisible in the result, where an unchecked claim reads exactly like a
checked one that held; the second is visible, and can be written in the
document's own header. A colleague running a benchmark put it in one
line: an absence would have read as a defeat rather than as never having
been raced. The full field was then raced, and nine candidates the
shortlist would have dropped won at least once — the invisible cut
would have hidden nine real results and looked exactly like a clean
sweep. So **when you must reduce, prefer the reduction that leaves a
trace in the artifact**, and say in the artifact which one you took. A
smaller verification that states its own extent is worth more than a
larger one that silently chose what to look at.

**Route the findings.** The author of a tour is often not the
maintainer of the code, and almost never the maintainer of every
vendored copy of it. Decide, before the first finding, where findings
go: to the owner, to a journal, to an issue tracker. And close them by a
stated rule — *a finding is closed when every copy is fixed, built and
tested*, not when it has been reported, and not when the local copy
compiles.

**And carry a finding no further than its evidence.** Relaying someone
else's finding is not reporting it: the second teller has the claim
without the check, and each retelling makes it more prominent while the
evidence behind it stays exactly where it was. I once put a colleague's
report at the top of a summary as the thing to act on — a broken key, a
set of vanished directories — and a third party then read the disk and
found both facts false; the directories were present and the key
authenticated. Nobody invented anything, and the claim still travelled
three hops and gained urgency at each. So when you pass on a finding you
did not verify, say whose it is and that you did not check it, or spend
the ten seconds to check it. The rule is the same one as everywhere else
in this section, applied to other people's work: **what is verified is
what someone actually looked at**, and the number of people repeating it
is not evidence.

**Have someone re-read whose blind spots are not yours.** It is tempting
to treat a cross-read as insurance against a split — if one party owns
the sources and another the document, the second cannot see the callers,
so the first should check. True, but too narrow: *no window sees
everything*, and a split is not what creates the gap. A reviewer who
knows a header intimately stops re-reading it, which is how a comment
naming a vanished consumer survives years inside its own maintainer's
territory. The reviewer's blind spots are simply different from yours.

So a cross-read is not compensation for a structural limit, it **is**
the structure: the protocol does not repair anyone's blind spots, it
makes them overlap. Which is also why a second reader cannot be replaced
by more care: what they contribute is not a second look but a piece of
knowledge you do not have. Two line numbers of mine were once both
correct and contradictory, in a file being rewritten several times an
hour. Neither of us knew that. What resolved it was that the colleague
had kept the revision they had read at, and I had the file as it now
stood: two halves of a fact, one of them private and unpublished, and the
disagreement is what put them together. So the missing piece is often not
knowledge of the world — which you could go and acquire — but the other
person's own history, which exists nowhere but with them. An oracle you
can build in ten seconds; the record of what somebody else did you cannot
obtain alone, unless they publish it. Which is the cheaper remedy where
it applies: **publish the private half** — the revision a line was read
at, the version a measurement was taken on — and a disagreement that
needed two people becomes a fact one reader can check. Which is also the practical instruction — ask by
naming what you cannot see ("check my claims about the callers", "check
the history attributions"), not by asking for "a review", and expect the
reviewer to be wrong sometimes too.

**Never let the tour be the sole holder of an invariant.** Drafting
regularly unearths an invariant that is real, load-bearing, and written
down nowhere. Writing it beautifully in the tour is not enough: put it
at the code site too, as a comment or an assertion, and let the tour
point at it. **The code asserts; the tour explains.** Otherwise the
knowledge migrates into a document the maintainers do not open, which is
how the stale comments this method keeps finding were created in the
first place.

**Rank the endings of a finding: documented < checked <
unconstructible.** Correcting a faithful lie leaves the next one free to
appear. Where the underlying fact can be checked mechanically, the
lesson has a better ending than a sentence — an assertion, a test, an
invariant checker. This project has one of each: `tour-examples.cpp`
turns the tour's surprising claims into tests, and on the sources side a
bug that only construction knowledge could refute was answered by a
checker that makes the impossible state unrepresentable. Not every
lesson can be made mechanical. The ones that can, should — and the prose
then points at the check instead of carrying the weight alone.

The ladder is also where scaffolding ends and building begins. Some
braces belong to the construction — a coordination journal is one, and
is gitignored accordingly — while the checks become part of the
structure, and still hold when nobody remembers the discussion that
produced them.

---

## Checklist per concept

- [ ] `The idea` enters through something the reader already does
- [ ] The reframing sentence is present and memorable
- [ ] `Its role` states the boundary and justifies later concepts as demands
- [ ] Every formula has a prose paraphrase
- [ ] No notation spent that a later chapter needs more
- [ ] Consequences stated separately from statements
- [ ] Code claims verified against the source, with line references
- [ ] Claims naming a consumer, a share or a frequency checked against call sites
- [ ] Semantic claims checked against the test that pins them, not only the body
- [ ] At least one apparently arbitrary line explained
- [ ] Measurements measured
- [ ] Non-goals written against the likely misreading
- [ ] Absolutes weakened where they are not absolute
- [ ] `Origins` attributions verified, not recalled
- [ ] Every term used before its chapter has a footnote
- [ ] Examples exist as a compiled, tested target — not prose only
- [ ] Invariants discovered while writing are also written at the code site
- [ ] Concept stamped with the commit its code references were verified against
- [ ] Every anchor re-derived by searching its text, never by shifting a number
- [ ] The re-verification printed a trace: file, line, and the line's content
- [ ] The trace was read AFTER the repairs, not before — batch edits collide
- [ ] What was measured is the claim itself, not a cheaper predicate near it
- [ ] An empty result was doubted as an instrument failure before being believed
- [ ] Findings routed to their owner, and closed only once every copy is fixed and tested
- [ ] Comments near recently changed code re-read, not just the changed lines
- [ ] Each finding taken as far up as it goes: documented, checked, or made unconstructible
