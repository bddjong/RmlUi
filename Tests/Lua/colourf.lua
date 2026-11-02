local function fails(fn) assert(not pcall(fn)) end
local unpack_values = table.unpack or unpack
local fields = {'red', 'green', 'blue', 'alpha'}
local c = Colourf.new('0', 0.5, 1, '2.5')
local function check(r, g, b, a)
    local values = {r, g, b, a}
    local rgba = c.rgba
    assert(type(rgba) == 'table')
    for i, field in ipairs(fields) do
        assert(c[field] == values[i] and rgba[i] == values[i])
    end
end
check(0, 0.5, 1, 2.5)
c.rgba = {'-2', 0, '1', 256.5}
check(-2, 0, 1, 256.5)
local copy = c.rgba
copy[1] = 42
check(-2, 0, 1, 256.5)
c.rgba = c.rgba
check(-2, 0, 1, 256.5)
for i, field in ipairs(fields) do c[field] = tostring(i / 2) end
check(0.5, 1, 1.5, 2)
for _, value in ipairs({false, 'bad', {}}) do
    for i, field in ipairs(fields) do
        local args = {1, 2, 3, 4}
        args[i] = value
        fails(function() Colourf.new(unpack_values(args)) end)
        fails(function() c[field] = value end)
        fails(function() c.rgba = args end)
        check(0.5, 1, 1.5, 2)
    end
end
for i = 1, 4 do
    local args = {1, 2, 3, 4}
    args[i] = nil
    fails(function() c.rgba = args end)
    check(0.5, 1, 1.5, 2)
end
fails(function() c.rgba = 42 end)
check(0.5, 1, 1.5, 2)
-- Existing Lua numeric conversion accepts infinities and NaN on all paths.
local special = {math.huge, -math.huge, 0/0}
for _, value in ipairs(special) do
    local function same(x) return x == value or (x ~= x and value ~= value) end
    assert(same(Colourf.new(value, 0, 0, 0).red))
    c.red = value
    assert(same(c.red))
    c.rgba = {0, 0, 0, value}
    assert(same(c.rgba[4]))
end
-- Byte and float colors share table shape, snapshot, and atomic failure semantics.
for _, make in ipairs({Colourb.new, Colourf.new}) do
    local color = make(1, 2, 3, 4)
    local t = color.rgba
    color.rgba = t
    t[1] = 9
    assert(color.red == 1)
    fails(function() color.rgba = {5, 6, false, 8} end)
    for i, field in ipairs(fields) do assert(color[field] == i) end
end
