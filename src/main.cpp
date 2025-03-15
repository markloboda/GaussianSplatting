#include <Application/Application.h>

int main() {
   Application app;
   if (!app.Initialize()) {
      return 1;
   }
   app.Run();
   app.Terminate();

   return 0;
}
