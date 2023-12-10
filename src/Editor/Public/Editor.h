#pragma once

namespace Zn
{
class Editor
{
  public:
    static void Create();
    static void Destroy();
    static void Tick(float deltaTime_);

  private:
    Editor()  = default;
    ~Editor() = default;
};
} // namespace Zn
