Fast Method Source changelog
============================

### v0.4.0 (June 18, 2015)

* _Significantly_ improve speed of both `#soruce` and `#comment`. The trade-off
is that you must indent methods properly.
* Decrease memeory consumption

### v0.3.1 (June 11, 2015)

* Considerable speed improvements

### v0.3.0 (June 11, 2015)

* _Significantly_ reduced memory consumption (`#comment_and_source` for 19K
methods uses about 100-150 MB of RES RAM)
* _Significantly_ decreased time of calculations

### v0.2.0 (June 11, 2015)

* Significantly reduced memory consumption
* The time of calculations has increased by 6%
* Fixed memory not being freed on exceptions
* Fixed exit code value for shells [→](https://github.com/kyrylo/fast_method_source/pull/2)

### v0.1.1 (June 9, 2015)

* Fixed segfault on any C method query

### v0.1.0 (June 9, 2015)

* Initial release
