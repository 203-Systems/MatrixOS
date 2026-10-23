#define MATRIXOS_INSTALLER_TEST
#include "../../OS/Commands/AppInstaller.cpp"
#include <fstream>
#include <iterator>
using namespace MatrixOS;
vector<uint8_t> response;
bool Capture(const vector<uint8_t>& data,bool,void*) { response=data; return true; }
vector<uint8_t> SendCommand(uint8_t sub, uint32_t session, const vector<uint8_t>& data={}) {
  static unsigned id=0; ++id;
  vector<uint8_t> r{0x70,sub,uint8_t(id>>8),uint8_t(id)};
  AppInstaller::Put32(r,session); r.push_back(data.size()); r.insert(r.end(),data.begin(),data.end());
  assert(AppInstaller::Handle(r.data(),r.size(),Capture)); return response;
}
void Write(const string& logical,const string& text) {
  fs::create_directories(fs::path(Physical(logical)).parent_path());
  std::ofstream out(Physical(logical)); out<<text;
}
string Read(const string& logical) { std::ifstream in(Physical(logical)); return string(std::istreambuf_iterator<char>(in),{}); }
int main(int argc,char** argv) {
  assert(argc==3); testRoot=argv[1]; fs::create_directories(testRoot);
  std::ifstream input(argv[2],std::ios::binary);
  vector<uint8_t> pkg((std::istreambuf_iterator<char>(input)),{}); assert(pkg.size()>24);
  vector<uint8_t> rejectedArgs(16, 42); rejectedArgs.push_back(0);
  AppInstaller::Put32(rejectedArgs,pkg.size()); AppInstaller::Put32(rejectedArgs,0);
  FileSystem::available=false;
  assert(SendCommand(1,0,rejectedArgs)[8]==AppInstaller::Busy && AppInstaller::Get().state!=AppInstaller::Receiving);
  FileSystem::available=true; SYS::activeAppId=OS_SHELL;
  USB::mode=USB_MODE_MSC;
  assert(SendCommand(1,0,rejectedArgs)[8]==AppInstaller::Busy && AppInstaller::Get().state!=AppInstaller::Receiving);
  USB::mode=USB_MODE_NORMAL;
  auto upload=[&](bool replace) {
    vector<uint8_t> args(16,0); args[0]=uint8_t(++SYS::now); args.push_back(replace);
    AppInstaller::Put32(args,pkg.size()); AppInstaller::Put32(args,~AppInstallFormat::CRC(~0U,pkg.data(),pkg.size()));
    auto begin=SendCommand(1,0,args); assert(begin[8]==0);
    uint32_t session=AppInstallFormat::U32(begin.data()+4);
    assert((AppInstaller::Get().state==AppInstaller::Receiving));
    assert(SYS::activeAppId==AppInstaller::DeveloperId);
    auto repeated=SendCommand(1,0,args); assert(AppInstallFormat::U32(repeated.data()+4)==session);
    auto conflict=args; conflict.back()^=1;
    assert(SendCommand(1,0,conflict)[8]==AppInstaller::BadPayload);
    assert(SendCommand(2,session+1,{0,0,0,0,1})[8]==AppInstaller::BadSession);
    assert(SendCommand(2,session,{0,0,0,1,1})[8]==AppInstaller::BadOffset);
    assert(SendCommand(3,session)[8]==AppInstaller::BadPayload);
    for(size_t offset=0;offset<pkg.size();offset+=50) {
      vector<uint8_t> bytes; AppInstaller::Put32(bytes,offset);
      size_t size=std::min<size_t>(50,pkg.size()-offset);
      bytes.insert(bytes.end(),pkg.begin()+offset,pkg.begin()+offset+size);
      auto reply=SendCommand(2,session,bytes); assert(reply[8]==0);
      assert(AppInstallFormat::U32(reply.data()+10)==offset+size);
      if(offset==0) { // Old duplicates succeed; conflicting retransmissions do not.
        assert(SendCommand(2,session,bytes)[8]==0);
        bytes.back()^=1; assert(SendCommand(2,session,bytes)[8]==AppInstaller::BadOffset);
      }
    }
    return session;
  };
  uint32_t session=upload(false);
  SendCommand(3,session); assert(AppInstaller::Get().state==AppInstaller::Committed);
  assert(AppInstaller::Get().state==AppInstaller::Committed);
  assert(Read("/MatrixOS/Applications/TestApp/main.py")=="def loop():\n    pass\n");
  Write("/MatrixOS/AppData/keep.txt","user data");
  Write("/MatrixOS/Applications/TestApp/main.py","old version");
  session=upload(true);
  failRenameFrom=AppInstaller::Stage; // Failure between old->backup and stage->target.
  SendCommand(3,session); assert(AppInstaller::Get().state==AppInstaller::Failed);
  assert(Read("/MatrixOS/Applications/TestApp/main.py")=="old version");
  assert(Read("/MatrixOS/AppData/keep.txt")=="user data");
  assert(AppInstaller::Get().state!=AppInstaller::Receiving);
  // Simulate reset after old directory moved, with a valid prepared journal.
  fs::rename(Physical("/MatrixOS/Applications/TestApp"),Physical(AppInstaller::Backup));
  fs::create_directories(Physical(AppInstaller::Stage));
  Write(AppInstaller::Journal,"{\"app\":\"TestApp\",\"hadOld\":true}");
  assert(AppInstaller::Recover());
  assert(Read("/MatrixOS/Applications/TestApp/main.py")=="old version");
  // Reset after promotion: keep new, remove old backup.
  Write(AppInstaller::Backup+"/old.txt","old");
  Write(AppInstaller::Journal,"{\"app\":\"TestApp\",\"hadOld\":true}");
  assert(AppInstaller::Recover()); assert(!fs::exists(Physical(AppInstaller::Backup)));
  session=upload(true); SendCommand(5,session); assert(AppInstaller::Get().state==AppInstaller::Aborted);
  assert(AppInstaller::Get().state!=AppInstaller::Receiving);
  // The existing app End() lifecycle cancels before a different app can use storage.
  session=upload(true); AppInstaller::Cancel(); SYS::activeAppId=OS_SHELL;
  assert(AppInstaller::Get().state==AppInstaller::Aborted && !AppInstaller::streamOpen);
  assert(SendCommand(5,session)[8]==0); // An already-aborted session needs no file I/O.
  session=upload(true); SYS::now+=600001; AppInstaller::Poll();
  assert(AppInstaller::Get().state==AppInstaller::Aborted && !AppInstaller::streamOpen);
  assert(SYS::launches>=2);
  // Package corruption is rejected before activation.
  pkg.back()^=1;
  session=upload(true); SendCommand(3,session);
  assert(AppInstaller::Get().state==AppInstaller::Failed);
  assert(Read("/MatrixOS/Applications/TestApp/main.py")=="old version");
  assert(AppInstaller::Get().state!=AppInstaller::Receiving);
  for(const auto& name:{"../x","/x","a\\b","CON.txt","a/","a.","a//b"}) assert(!AppInstallFormat::Path(name));
  std::puts("firmware installer: upload, duplicate, corruption, replace rollback, recovery, abort and data preservation passed");
}
