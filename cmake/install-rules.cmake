install(
    TARGETS dest_exe
    RUNTIME COMPONENT dest_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
