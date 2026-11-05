add_executable(itmoloops
    main.cpp
    proj.cpp
    audio_engine.cpp
)
target_link_libraries(itmoloops PRIVATE itmoloops_lib)
