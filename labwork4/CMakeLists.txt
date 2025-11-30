add_executable(
    hamarc
    main.cpp
    hamarc.cpp
    hamming.cpp
)

target_compile_features(hamarc PRIVATE cxx_std_20)
