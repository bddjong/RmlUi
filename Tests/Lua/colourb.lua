local function fails(fn)
    assert(not pcall(fn))
end

local c = Colourb.new(1, 2, 3, 4)
local function check(r, g, b, a)
    assert(c.red == r and c.green == g and c.blue == b and c.alpha == a)
    local rgba = c.rgba
    assert(type(rgba) == 'table')
    assert(rgba[1] == r and rgba[2] == g and rgba[3] == b and rgba[4] == a)
end

check(1, 2, 3, 4)
c.rgba = {0, 255, 30, 40}
check(0, 255, 30, 40)
local copy = c.rgba
copy[1] = 17
check(0, 255, 30, 40)
c.red, c.green, c.blue, c.alpha = 5, 6, 7, 8
check(5, 6, 7, 8)
c.rgba = c.rgba
check(5, 6, 7, 8)
c.red = '5' -- Lua integer conversion accepts numeric strings.
check(5, 6, 7, 8)
assert(Colourb.new(256, 0, 0, 0).red == 0)
c.red = -1
assert(c.red == 255)
c.red = 5

for _, value in ipairs({false, 'bad'}) do
    fails(function() Colourb.new(value, 2, 3, 4) end)
    for _, field in ipairs({'red', 'green', 'blue', 'alpha'}) do
        fails(function() c[field] = value end)
        check(5, 6, 7, 8)
    end
    for i = 1, 4 do
        local rgba = {10, 20, 30, 40}
        rgba[i] = value
        fails(function() c.rgba = rgba end)
        check(5, 6, 7, 8)
    end
end
fails(function() c.rgba = {10, 20, 30} end)
fails(function() c.rgba = 42 end)
check(5, 6, 7, 8)
