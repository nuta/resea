# Porting

Porting Resea to another CPU would not be hard work as it isolates arch-specific
stuff as much as possible. What you'll have to implement at least are:

- `kernel/arch/<arch name>`
- `libs/resea/arch/<arch name>`

I'm looking forward to your pull request :)
