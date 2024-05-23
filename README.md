# flutter_desktop_scanner

Making scanning with physical scanners on desktop a breeze!

## Getting Started

Because this plugin depends on some system libraries (SANE, WIA, TWAIN) you have to set up them up for linking in your flutter project.  

### Linux

<details>
<summary>Here's how to setup linking</summary>

In `./linux/CMakeLists.txt` under 
  
```cmake
apply_standard_settings(${BINARY_NAME})
```

add:

```cmake
set(LINKER_FLAGS "-lsane")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
```

</details>