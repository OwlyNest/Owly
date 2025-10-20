# CHANGELOG.md

## [0.0.1] — *“The Owl Is Born”*  
*Release date: 2025-06-12*  

Behold, from the haunted depths of cursed syntax and C-induced sleep deprivation, **Owly** emerges.  
Feathers are still wet. Sanity? Questionable. But it compiles.

---

### Added

- **The Sacred Ritual (`func main()`)**
  - Owly now understands where code begins. Praise be the void, and pass the file descriptor.

- **`print()` statement support**
  - Strings? Printed.  
  - Integers? Screeched into the void.  
  - Lists? Beautiful arrays of nonsense, spat out like regurgitated pellets.

- **Variable Declarations**
  - `int` and `char` now exist. They do nothing special. They just are. Like owls in a tree.
  - `list` of strings (`const char*[]`) now supported. Technically a `const char*` rave.

- **Flow Control**
  - `if`: Decides.  
  - `else if`: Keeps deciding.  
  - `else`: Gives up and runs whatever’s left.  
  - All brackets placed gently where C expects them, lest we anger the compiler demon.

- **`while` Loops**
  - Owly now loops until it dies or breaks out — just like developers.

- **Output: C Code**
  - Automatically wraps your hoots in a proper C `main()` with `#include <stdio.h>`.
  - Owly doesn't indent code; it caresses it into shape with precision tabs and guilt.

---

### Internals

- Keeps track of open blocks like it’s herding owl eggs.
- `list_vars[]` stores known lists to ensure we treat them with special love and printf hacks.
- Everything assumes the input is valid. If it isn't? It’s your problem now. 🪦

---

### Known Nightmares

- No support for user-defined functions yet. Owly is still too young to recurse.
- No `for` loops yet. Those are for mature birds.
- Errors are not errors — they’re prophecies in disguise. Expect segmentation faults if you anger it.

---

### Philosophy

Owly isn't just a language. It’s a *lifestyle*.  
You don't write Owly code. You **whisper it** into the compiler and pray the spirit of Dennis Ritchie doesn’t smite you.  
Future versions will feature:  
- `hoot()` for expressive printing  
- `owl()` for mystical error messages  
- And maybe a standard library if we survive.

---

> “It compiled on the first try. I wept.”  
— Anonymous user, probably lying
---
## [0.1.0] — *“The Owl Is Grown”*  
*Release date: 2025-06-14* 

---
### Added
- **function declaration**
  - Easy to do, `func your_func_name() {}`
  - The owl is quite lazy, she only supports void functions
  - Owly prefers it if you define your function before using it in the main. Your code will still work, she'll just be a bit annoyed. Expect screetches (warnings)
- **for loops**
  - Yes, C style, for the python users reading this: haha, good luck!
  - But seriously, use for (int i = [start]; i < [stop]; i = i + 1) {} or whatever suits your mood
  - Owly is not a fan of snakes so python style for loops are not supported. Maybe in the future she'll be open to it
  - pyfor or forin maybe in the future?
---
## [2.0.0-pre] - *"The Owl Can Read (a little)"*
*Release date: 4-7-2025*
---
She's back — heavier, moodier, and armed with a lexer. **Owly v2** has hatched from the ashes of **v1**   like cryptic phoenix. She's learned to read, but comprehension is... aspirational.
---
### Added
- **`Owlyc2.c` — The Compiler Reborn**
  - 125.5% bigger. 82% angrier. Function declarations? Still missing.
  - Think of it as a large, feathered promise. A compiler that dreams big, crashes hard.
- **`Owlylexer.c` — Now With Extra Glyph Deciphering**
  - Behold, the great tokenizer: it shreds your code into lexical confetti and logs each piece with divine alignment.
  - Example:
  `[DEBUG]: token = { type: KEYWORD_FUNC            , lexeme: 'func'     }`
  - Yes, those debug lines are deliciously symmetrical. Yes, they will blind you at 3AMYes, those debug lines are deliciously symmetrical. Yes, they will blind you at 3AM.
  - Perfect for when you want to know exactly where things went wrong, but also kind of want to suffer.
- **The Ritual of Execution (now with more flags)**  
  - Run your owly incantations via:
  ```bash
  chmod +x owlyrun.sh
  ./owlyrun.sh [-v|--verbose] [-n|--nodelete] [-d|--debug] <yourfile>.owly
  ```
  - Flags let you choose your flavor of pain. Want verbosity? Want lingering temp files? Want to stare deep into the debug void? It's all here.

  - Void function declarations. Which means the core is ready and owlyc3 will soon be a reality. I hope
### fixed
- Ghost Box Removal (Shell edition)
  - A mysterious white rectangle haunted the corner of Perch (the shell). It’s gone now. We don’t speak of it.
  - Fullscreen mode is now less cursed. Credit to CSS, the unsung exorcist.
- Maybe Other stuff
  - Honestly? If it broke before and doesn’t now, that’s a win. Don't ask too many questions.
---

The owl is learning. Slowly. Reluctantly. With many loud screeches.
v2 is a rebirth — not perfect, but promising. A lexing, compiling featherbeast that will one day rule your C pipeline like the eldritch queen she is.

---
> “I ran it and nothing exploded. Feels weird.”
— Same anonymous user, still probably lying
---