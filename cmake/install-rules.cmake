install(
    TARGETS m3_exe
    RUNTIME COMPONENT m3_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
