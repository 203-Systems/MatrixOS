#ifndef REGISTER_APPLICTION_H

#define APPLICATION_HELPER_CLASS_IMPL(CLASS) CLASS##_HELPER
#define APPLICATION_HELPER_CLASS(CLASS) APPLICATION_HELPER_CLASS_IMPL(CLASS)

#define APPLICATION_HELPER_CLASS_OBJ_IMPL(CLASS) CLASS##_HELPER_OBJ
#define APPLICATION_HELPER_CLASS_OBJ(CLASS) APPLICATION_HELPER_CLASS_IMPL(CLASS)

#define APPLICATION_INFO_IMPL(CLASS) CLASS##_INFO
#define APPLICATION_INFO(CLASS) APPLICATION_INFO_IMPL(CLASS)

#define APPLICATION_INDEX 0
inline uint16_t registered_app_count = 0; //Would love to be able to free this after initialization 

#define REGISTER_APPLICTION_H
#endif

#ifndef APPLICATION_VERSION
#define APPLICATION_VERSION 0
#endif

#ifndef APPLICATION_VISIBLITY
#define APPLICATION_VISIBLITY true
#endif
 
// #pragma message ("APP <" APPLICATION_NAME "> Registered")

extern Application_Info* applications[];

inline Application_Info APPLICATION_INFO(APPLICATION_CLASS) = Application_Info(
                                                                              StaticHash(APPLICATION_AUTHOR "-" APPLICATION_NAME),
                                                                              APPLICATION_NAME,
                                                                              APPLICATION_AUTHOR,
                                                                              APPLICATION_COLOR,
                                                                              APPLICATION_VERSION,
                                                                              [&]() -> Application*{ return new APPLICATION_CLASS();},
                                                                              APPLICATION_VISIBLITY);

__attribute__ ((__constructor__))
inline void APPLICATION_HELPER_CLASS(APPLICATION_CLASS) (void)
{
  //For some reason this function is called multiple times, so check if application already registered. 
  //TODO: Need to fix
  for(uint8_t i = 0; i < registered_app_count; i++) 
  {
    if(applications[i] == &APPLICATION_INFO(APPLICATION_CLASS))
      return;
  }
  applications[registered_app_count] = &APPLICATION_INFO(APPLICATION_CLASS);
  ESP_LOGI("APP REG", APPLICATION_NAME " Registered at %d", registered_app_count);
  registered_app_count++;
}

#pragma push_macro( "APPLICATION_INDEX" )
#undef APPLICATION_INDEX
#define APPLICATION_INDEX _Pragma("pop_macro(\"APPLICATION_INDEX\")") APPLICATION_INDEX + 1

#undef APPLICATION_NAME
#undef APPLICATION_AUTHOR
#undef APPLICATION_COLOR
#undef APPLICATION_VERSION
#undef APPLICATION_VISIBLITY
#undef APPLICATION_CLASS
