-- Snake for the 152x86 Lua GUI canvas.
-- UP=up, DOWN=down, A=right, B=left.
local cols, rows, cell = 24, 12, 6
local offsetX, offsetY = 4, 3
local snake = {{x=12,y=6}, {x=11,y=6}, {x=10,y=6}}
local dx, dy = 1, 0
local food = {x=18, y=6}
local score = 0
local alive = true

math.randomseed(rf.millis())

local function on_snake(x, y)
    for _, part in ipairs(snake) do
        if part.x == x and part.y == y then return true end
    end
    return false
end

local function place_food()
    repeat
        food.x = math.random(0, cols - 1)
        food.y = math.random(0, rows - 1)
    until not on_snake(food.x, food.y)
end

local function box(x, y, color)
    rf.gui_rect(offsetX + x * cell, offsetY + y * cell,
                cell - 1, cell - 1, color, true)
end

rf.gui_begin("LUA SNAKE")
rf.gui_footer("B LEFT", "U/D", "A RIGHT")

while alive do
    -- Reject immediate 180-degree turns.
    if rf.button("up") and dy ~= 1 then dx, dy = 0, -1
    elseif rf.button("down") and dy ~= -1 then dx, dy = 0, 1
    elseif rf.button("a") and dx ~= -1 then dx, dy = 1, 0
    elseif rf.button("b") and dx ~= 1 then dx, dy = -1, 0
    end

    local head = {x=snake[1].x + dx, y=snake[1].y + dy}
    if head.x < 0 or head.x >= cols or head.y < 0 or head.y >= rows or
       on_snake(head.x, head.y) then
        alive = false
    else
        table.insert(snake, 1, head)
        if head.x == food.x and head.y == food.y then
            score = score + 1
            place_food()
        else
            table.remove(snake)
        end

        rf.gui_clear()
        box(food.x, food.y, "red")
        for index, part in ipairs(snake) do
            box(part.x, part.y, index == 1 and "accent" or "green")
        end
        rf.gui_text(116, 75, "S:" .. tostring(score), "yellow")
        rf.delay(math.max(55, 150 - score * 4))
    end
end

rf.gui_clear()
rf.gui_text(47, 25, "GAME OVER", "red")
rf.gui_text(50, 39, "SCORE " .. tostring(score), "yellow")
rf.gui_text(33, 56, "A=REPLAY B=LIST", "accent")
rf.gui_footer("B LIST", "", "A REPLAY")
print("Snake score", score)
