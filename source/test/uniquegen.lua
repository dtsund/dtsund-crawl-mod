-- Test that randomly generated monsters aren't uniques.

local place = dgn.point(20, 20)
local count = 100

local function test_place_random_monster()
  dgn.dismiss_monsters()
  dgn.grid(place.x, place.y, "floor")
  m = dgn.create_monster(place.x, place.y, "random")
  assert(not m.unique,
         "random monster is unique " .. m.name)
end

local function test_random_unique(branch, depth)
  crawl.message("Running random monster unique tests in branch " .. branch)
  debug.flush_map_memory()
  for d = 1, depth do
    debug.goto_place(branch .. ":" .. d)
    for i = 1, count do
      test_place_random_monster()
    end
  end
end

local function run_random_unique_tests()
  for depth = 1, 27 do
    test_random_unique("D", depth, 3)
  end

  for depth = 1, 7 do
    test_random_unique("Dis", depth, 3)
  end

  for depth = 1, 5 do
    test_random_unique("Swamp", depth, 5)
  end
end

run_random_unique_tests()
