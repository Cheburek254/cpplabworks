add_executable(itmoloops
    main.cpp
    engine.cpp
    parser.cpp
    wav.cpp
    instruments.cpp
    effects.cpp
)
target_link_libraries(itmoloops PRIVATE itmoloops_lib)
