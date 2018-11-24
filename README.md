# Loaf

Loaf is a programming language.

It's inspired by Go, parts of Ruby, and a bit of C.

I'm writing it to learn how these sort of things work.

## Goals

- Functions
  - [x] Store/retrieve globals
    - [x] Create Value table
  - [x] Call frames
  - [x] Execute parameterless, return value-less functions (ie. procedures)
  - [ ] Parameters
  - [ ] Return values
- Make some new control flow statements
  - [ ] For loop
  - [ ] While loop
  - [ ] Else/else-if statements
- Proper type system
- Get structs to work
- Operator overloading
- Compile to WebAssembly

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
