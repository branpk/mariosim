### SM64 Movement Simulator and Brute Forcer

This project is in an immature state, as it only supports one frame of crouch sliding.
More features will be added on request, or as the decomp progresses.

This is a command line program that takes the name of an input file as argument.

The input file has the following basic syntax:

```
mario = {
  facingYaw = 9988
  slidingYaw = 8000
  hSpeed = 22.125
  slidingSpeed = {15.354, 15.931}
}

floor = {
  type = 0x15
  normal = {-0.0005521466, 0.8594093919, 0.5112877488}
}

camera = {
  angle = 0xF327
}

input = solve(hSpeed = maxlt(22.5))
```

Numbers can be written in hex or decimal. For floating point values, you can also use the IEEE 754 hex value.

You can replace `maxlt(22.5)` with one of the following goals:
```
  hSpeed = <value>          -   get hSpeed equal to <value> exactly
  hSpeed = max()            -   maximize hSpeed
  hSpeed = maxlt(<value>)   -   maximumize hSpeed less than <value>
  hSpeed = near(<value>)    -   get hSpeed as close as possible to <value>
```

If the program succeeds in reaching the goal, it will print out the stick x and y that it used, as well as the resulting mario state.
Note that the stick y value matches the N64 convention, that negative is down and positive is up. TAS input expects the opposite.

Let me know if you want any additional features or if there is any error (incorrect simulation, doesn't find optimal input, etc).
