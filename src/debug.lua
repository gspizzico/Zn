local zb_path = "F:/ZeroBraneStudio"

local cpaths = {
    --string.format("%s/bin/x64/clibs/?.dll;%s", zb_path, zb_path),
    --string.format("%s/bin/x64/lib?.dll;%s/bin/clibs53/?.dll;", zb_path, zb_path),
    --string.format("%s/bin/x64/lib?.dll;%s/bin/x64/clibs53/?.dll;", zb_path, zb_path),
    package.cpath,
}
package.cpath = table.concat(cpaths, ';')

local paths = {
    string.format('%s/lualibs/?.lua;%s/lualibs/?/?.lua', zb_path, zb_path),
    string.format('%s/lualibs/?/init.lua;%s/lualibs/?/?/?.lua', zb_path, zb_path),
    string.format('%s/lualibs/?/?/init.lua', zb),
    package.path,
}
package.path = table.concat(paths, ';')

require('mobdebug').start()