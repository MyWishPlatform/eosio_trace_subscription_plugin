Clone repo:
```bash
eos/plugins]$ git clone https://github.com/MyWishPlatform/eosio_trace_subscription_plugin/
eos/plugins]$ mv eosio_trace_subscription_plugin trace_subscription_plugin
```

<br />

Modify eos/plugins/CMakeLists.txt:
```
...
add_subdirectory(trace_subscription_plugin)
...
```

<br />

Modify eos/programs/nodeos/CMakeLists.txt:
```
...
PRIVATE -Wl,${whole_archive_flag} trace_subscription_plugin  -Wl,${no_whole_archive_flag}
...
```

<br />

Compile:
```bash
eos/build]$ make
eos/build]$ sudo make install
```

<br />

Add to config.ini:
```
...
plugin = eosio::trace_subscription_plugin
...
```
