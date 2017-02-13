-- These will be provided by engine
function LegacyEngine()
	local self = {}

	function self.registerFont(name, size)
		return 0
	end

	function self.getShader(name)
		return 0
	end

	function self.drawRect(x, y, w, h, color, filled)
	end

	function self.drawShader(x, y, w, h, shader)
	end

	function self.drawText(text, x, y, font, size, colorized, alpha)
	end

	function self.drawTextWithCursor(text, x, y, cursorpos, font, size, colorized, alpha)
	end

	function self.textWidth(text, font, size, colorized)
		return 10
	end

	function self.textHeight(text, font, size, colorized)
		return 10
	end

	function self.getText(text)
		return text
	end

	function self.getVersion()
		return "2.73"
	end

	-- set the current visible window
	function self.setVisible( component )
	end

	function self.print(value)
		print(value)
	end

	return self
end

if _G.etl == nil then
    -- we do this here so that we can test script in a normal lua binary
    _G.etl = LegacyEngine()
end

-- The ui Base written by Jere 'Jacker' S.
-- Coding style is camelCase and private methods are marked with '_'
-- or essentially methods that should not be called directly

function UIComponent()
	local self = {}

	-- Init the default values
	self.width = 0
	self.height = 0
	self.posx = 0
	self.posy = 0
	self.parent = nil
	self.hidden = false
	self.children = {}

	function self.setWidth( value )
		self.width = value
	end

	function self.setHeight( value )
		self.height = value
	end

	function self.getHeight()
		return self.height
	end

	function self.getWidth()
		return self.width
	end

	function self.setX( value )
		self.posx = value
	end

	function self.setY( value )
		self.posy = value
	end

	function self.getX()
		return self.posx
	end

	function self.getY()
		return self.posy
	end

	function self.setParent( component )
		self.parent = component
	end

	function self.getParent()
		return self.parent
	end

	function self.isHidden()
		return self.hidden
	end

	function self.setHidden( value )
		self.hidden = value
	end

	function self.addChild( component )
		component.setParent(self)
		table.insert(self.children, component)
	end

	function self.getChildren()
		return self.children
	end

	function self.removeAllChildren()
		for k in pairs (self.children) do
			self.children[k] = nil
		end
	end

	function self.getChildCount()
		--return table.getn(self.children)
		local count = 0
		for _ in pairs(self.children) do count = count + 1 end
		return count
	end

	-- this gets the components location recursivly
	function self.getLocation()
		local x = self.posx
		local y = self.posy
		if self.parent ~= nil then
			local tmpx, tmpy = self.parent.getLocation()
			x = x + tmpx
			y = y + tmpy
		end
		return x, y
	end

	function self.draw()
		local x, y = self.getLocation()
		self._drawAt(x, y)
		for k in pairs (self.children) do
			self.children[k].draw()
		end
	end

	-- check that all data is up to date.. (refresh string values etc)
	function self.validate()
		_doValidate()
		for k in pairs (self.children) do
			self.children[k]._doValidate()
		end
	end

	function self._doValidate()
		-- this will be extended
	end

	function self._drawAt(x, y)
		-- body is empty on the base class
		etl.print("Ignoring draw call to base class: " .. x .. ", " .. y)
	end

	function self.toString()
		return "Height: " .. self.height .. " Width: " .. self.width ..
			" X: " .. self.posx .. " Y: " .. self.posy
	end

	return self
end

function Button()
	local self = UIComponent()

	function self._drawAt(x, y)
		etl.print("Would draw Button at: " .. x .. ", " .. y)
	end

	return self
end

function Label()
	local self = UIComponent()
	return self
end

function TextField()
	local self = UIComponent()
	return self
end

function CheckBox()
	local self = UIComponent()
	return self
end

function ComboBox()
	local self = UIComponent()
	return self
end

function Slider()
	local self = UIComponent()
	return self
end

function HorizontalSlider()
	local self = Slider()
	return self
end

function VerticalSlider()
	local self = Slider()
	return self
end

function Window(title)
	local self = UIComponent()

	local windowPrivate = {}

	function self.setTitle(title)
		windowPrivate.title = etl.getText(title)
	end

	function self.getTitle()
		return windowPrivate.title
	end

	function self.setAlwaysOnTop( value )
		windowPrivate.alwaysTop = value
	end

	function self.alwaysOnTop()
		return windowPrivate.alwaysTop
	end

	local parenttoString = self.toString
	function self.toString()
		return parenttoString() .. " Title: " .. windowPrivate.title
	end

	function self.window()
		return windowPrivate
	end

	self.setTitle(title)
	self.setAlwaysOnTop(false)
	return self
end

function MainWindow(title)
	local self = Window(title)

	function self._drawAt(x, y)
		etl.print("Would draw MainWindow with title " .. self.window().title .. " at: " .. x .. ", " .. y)
	end

	return self
end

function SubWindow(title)
	local self = Window(title)

	function self._drawAt(x, y)
		etl.print("Would draw SubWindow with title " .. self.window().title .. " at: " .. x .. ", " .. y)
	end

	return self
end

function PopupWindow(title)
	local self = Window(title)

	function self._drawAt(x, y)
		etl.print("Would draw PopupWindow with title " .. self.window().title .. " at: " .. x .. ", " .. y)
	end

	self.setAlwaysOnTop(true)
	return self
end
