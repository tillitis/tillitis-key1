# Test applications

All of these test apps are controlled through the USB CDC, typically
by running picocom or similar terminal program, like:

```
$ picocom /dev/ttyACM1
```

- `defaultapp`: Immediately resets the TKey with the intention to
  start an app from the client, replicating the behaviour of earlier
  generations.
- `testapp`: Runs through a couple of tests that are now impossible
  to do in the `testfw`.
- `reset_test`: Interactively test different reset scenarios.
- `testloadapp`: Interactively test management app things like
  installing an app (hardcoded for a small happy blinking app, see
  `blink.h` for the entire binary!) and to test verified boot.
