-- actually setup some UI shit
require "ui/UIBase"

function Background(shader)
    local self = {}

    function self.init(shader)
        self.usedShader = etl.getShader(shader)
    end

    function self.draw()
        etl.drawShader(0, 0, 800, 600, self.usedShader)
    end

    self.init(shader)
    return self
end

function Pointer(shader)
    local self = {}

    function self.init(shader)
        self.shader = etl.getShader(shader)
    end

    function self.draw()
        -- get the pointer x & y location
        -- note: placeholders, transfer the date more nicely in the future
        etl.drawShader(etl.cursorX, etl.cursorY, 32, 32, self.shader)
    end

    self.init(shader)
    return self
end

function MainMenu()
    local self = {}

    function self.init()

    end

    function self.draw()

    end

    self.init()
    return self
end

function UIHandler()
    local self = {}
    self.menus = {}

    function self.init()
        --self.backgroundShader = etl.getShader("ui/assets/et_clouds")
        self.background = Background("ui/assets/et_clouds")
        self.pointer = Pointer("ui/assets/3_cursor3")
        self.menus.main = MainMenu()
    end
    
    function self.drawMain()
        -- etl.print("Well at the least this works\n")
        --etl.drawShader(0, 0, 800, 600, self.backgroundShader)
        self.background.draw()
        self.menus.main.draw()

        -- draw the pointer at the top of everything else
        self.pointer.draw()
    end

    self.init()
    return self
end

function RunUi()
    -- etl.print("Toimii eka1\n")
    uihan.drawMain()
end

uihan = UIHandler()
