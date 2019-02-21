# Loaf

Loaf is a toy programming language I am currently hacking on to learn some new things. It features a hand written lexer and parser that outputs to a custom bytecode, which is then ran on a stack based virtual machine. Eventually I want to make it output to WebAssembly.

The syntax is a bit of Go, a bit of Ruby and a bit of C.

## Building/running

```
cd src

./all.bash
```

Or alternatively run the `build.bash` and `run.bash` scripts as you need.

## Goals

- Type system
  - [x] Properly scope types
  - [x] Do a basic static type check for builtin types (number and bool)
  - [x] Typecheck functions
  - [ ] Type aliases
  - [ ] Custom type support (structs)
- Functions
  - [x] Store/retrieve globals
    - [x] Create Value table
  - [x] Call frames
  - [x] Execute parameterless, return value-less functions (ie. procedures)
  - [x] Add parameters
  - [x] Return values
- More language features
  - [x] `var` statement to declare variable by type with a default value
  - [x] `&&` and `||`
  - [x] `>=` and `<=`
  - [ ] Call frames in dynamic list
  - [ ] Integer type
  - [ ] For loop
  - [ ] While loop
  - [ ] Else/else-if statements
  - [ ] Go style multi-statement if statements
- Another compilation target
  - [ ] WebAssembly

## Spec

This is a brief rundown of what the language actually is:

```
// Comments
/* /* nested comments */ */

// Implicit declaration and assignment
x := 100

// Explicit declaration and assignment
var y int = 100

if x == 100 {
  println("Yay!")
}

struct User {
  // public by default fields
  id       int
  username string

  private

  // private fields
  password string
}

// all fields set to their 0 value (0 for ints, "" for strings)
u := User{}

// give public variables values at initialisation
u2 := User{
  id: 101,
  username: "paked"
}

// can have '?' or '!' at end of identifiers, ? implies a boolean return, ! implies mutation
func ready?() bool {
  return false;
}

// declare a 'me?' function on User
func (u *User) me?(id int) bool {
  return u.id == id
}

if u.me?(101) {
  println("I am 101!")
}
```
