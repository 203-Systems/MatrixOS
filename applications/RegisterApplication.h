#ifndef APPLICATION_VERSION
#define APPLICATION_VERSION 0
#endif

#ifndef APPLICATION_VISIBLITY
#define APPLICATION_VISIBLITY false
#endif

extern map<uint32_t, Application_Info> applications;

class APPLICATION_CLASS##Helper
{
  public:
  APPLICATION_CLASS##Helper()
  {
    applications[StaticHash(AUTHOR+"-"+NAME)] = Application_Info(
      APPLICATION_NAME,
      APPLICATION_AUTHOR,
      APPLICATION_NAME,
      APPLICATION_VERSION,
      [](){return new APPLICATION_CLASS();},
      APPLICATION_VISIBLITY
    )
  }
}

APPLICATION_CLASS##Helper APPLICATION_CLASS##_helper();

#undef APPLICATION_NAME
#undef APPLICATION_AUTHOR
#undef APPLICATION_COLOR
#undef APPLICATION_VERSION
#undef APPLICATION_VISIBLITY
#undef APPLICATION_CLASS

