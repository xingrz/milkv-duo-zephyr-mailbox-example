## What's this

A Zephyr demo for [Milk-V Duo](https://milkv.io/duo) to show how to communicate
with the big core.

Check [milkv-duo/duo-examples](https://github.com/milkv-duo/duo-examples/tree/main/mailbox-test)
for the Linux side example.

## Get started

```bash
west init -l app
west update
west build -s app -b milkv_duo -p
```

## LICENSE

[WTFPL](LICENSE)
