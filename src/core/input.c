#include "../core.h"

ENUM(MappingKind)
{
	KEY_TO_BUTTON,
	KEY_TO_AXIS,
	MOUSE_BUTTON_TO_BUTTON,
	MOUSE_BUTTON_TO_AXIS,
	CONTROLLER_BUTTON_TO_BUTTON,
	CONTROLLER_BUTTON_TO_AXIS,
	CONTROLLER_AXIS_TO_BUTTON,
	CONTROLLER_AXIS_TO_AXIS,
};

STRUCT(Mapping)
{
	MappingKind kind;
	union
	{
		KeyboardKey key;
		MouseButton mouseButton;
		GamepadButton controllerButton;
		GamepadAxis controllerAxis;
	} from;
	union
	{
		InputButton *button;
		InputAxis *axis;
	} to;
	union
	{
		struct
		{
			float xWhenPressed;
			float yWhenPressed;
		};
		struct
		{
			float dotX;
			float dotY;
			float threshold;
		};
	};
};

static List(Mapping) mappings;

static void UpdateButtonFromButton(InputButton *button, bool isDown, bool wasPressed, bool wasReleased)
{
	button->isDown |= isDown;
	button->wasPressed |= wasPressed;
	button->wasReleased |= wasReleased;
}

static void UpdateAxisFromButton(InputAxis *axis, bool isDown, float xWhenPressed, float yWhenPressed)
{
	if (isDown)
	{
		axis->position.x += xWhenPressed;
		axis->position.y += yWhenPressed;
	}
}

static void UpdateButtonFromAxis(InputButton *button, float x, float y, float dotX, float dotY, float threshold)
{
	bool down = x * dotX + y * dotY >= threshold;
	if (not down and button->isDown)
		button->wasReleased |= true;
	else if (down and not button->isDown)
		button->wasPressed |= true;
	button->isDown = down;
}

static void UpdateAxisFromAxis(InputAxis *axis, float x, float y)
{
	axis->position.x += x;
	axis->position.y += y;
}

void MapKeyToInputButton(KeyboardKey key, InputButton *button)
{
	ListAdd(&mappings, ((Mapping)
	{
		.kind      = KEY_TO_BUTTON,
		.from.key  = key,
		.to.button = button
	}));
}

void MapKeyToInputAxis(KeyboardKey key, InputAxis *axis, float xWhenPressed, float yWhenPressed)
{
	ListAdd(&mappings, ((Mapping)
	{
		.kind         = KEY_TO_AXIS,
		.from.key     = key,
		.to.axis      = axis,
		.xWhenPressed = xWhenPressed,
		.yWhenPressed = yWhenPressed,
	}));
}

void MapMouseButtonToInputButton(MouseButton mouseButton, InputButton *button)
{
	ListAdd(&mappings, ((Mapping)
	{
		.kind             = MOUSE_BUTTON_TO_BUTTON,
		.from.mouseButton = mouseButton,
		.to.button        = button,
	}));
}

void MapMouseButtonToInputAxis(MouseButton mouseButton, InputAxis *axis, float xWhenPressed, float yWhenPressed)
{
	ListAdd(&mappings, ((Mapping)
	{
		.kind             = MOUSE_BUTTON_TO_AXIS,
		.from.mouseButton = mouseButton,
		.to.axis          = axis,
		.xWhenPressed     = xWhenPressed,
		.yWhenPressed     = yWhenPressed,
	}));
}

void MapControllerButtonToInputButton(GamepadButton controllerButton, InputButton *button)
{
	ListAdd(&mappings, ((Mapping)
	{
		.kind                  = CONTROLLER_BUTTON_TO_BUTTON,
		.from.controllerButton = controllerButton,
		.to.button             = button
	}));
}

void MapControllerButtonToInputAxis(GamepadButton controllerButton, InputAxis *axis, float xWhenPressed, float yWhenPressed)
{
	ListAdd(&mappings, ((Mapping)
	{
		.kind                  = CONTROLLER_BUTTON_TO_AXIS,
		.from.controllerButton = controllerButton,
		.to.axis               = axis,
		.xWhenPressed          = xWhenPressed,
		.yWhenPressed          = yWhenPressed,
	}));
}

void MapControllerAxisToInputButton(GamepadAxis controllerAxis, InputButton *button, float dotX, float dotY, float threshold)
{
	ListAdd(&mappings, ((Mapping)
	{
		.kind                = CONTROLLER_AXIS_TO_BUTTON,
		.from.controllerAxis = controllerAxis,
		.to.button           = button,
		.dotX                = dotX,
		.dotY                = dotY,
		.threshold           = threshold
	}));
}

void MapControllerAxisToInputAxis(GamepadAxis controllerAxis, InputAxis *axis)
{
	ListAdd(&mappings, ((Mapping)
	{
		.kind                = CONTROLLER_AXIS_TO_AXIS,
		.from.controllerAxis = controllerAxis,
		.to.axis             = axis,
	}));
}

void UpdateInputMappings(void)
{
	int numMappings = ListCount(mappings);
	for (int i = 0; i < numMappings; ++i)
	{
		Mapping map = mappings[i];
		switch (map.kind)
		{
			case KEY_TO_BUTTON:
			case MOUSE_BUTTON_TO_BUTTON:
			case CONTROLLER_BUTTON_TO_BUTTON:
			case CONTROLLER_AXIS_TO_BUTTON:
			{
				map.to.button->isDown = false;
				map.to.button->wasPressed = false;
				map.to.button->wasReleased = false;
			} break;

			case KEY_TO_AXIS:
			case MOUSE_BUTTON_TO_AXIS:
			case CONTROLLER_BUTTON_TO_AXIS:
			case CONTROLLER_AXIS_TO_AXIS:
			{
				map.to.axis->position.x = 0;
				map.to.axis->position.y = 0;
			} break;
		}
	}

	for (int i = 0; i < numMappings; ++i)
	{
		Mapping map = mappings[i];
		switch (map.kind)
		{
			case KEY_TO_BUTTON:
			{
				KeyboardKey key = map.from.key;
				UpdateButtonFromButton(map.to.button, IsKeyDown(key), IsKeyPressed(key), IsKeyReleased(key));
			} break;
			case KEY_TO_AXIS:
			{
				KeyboardKey key = map.from.key;
				UpdateAxisFromButton(map.to.axis, IsKeyDown(key), map.xWhenPressed, map.yWhenPressed);
			} break;
			case MOUSE_BUTTON_TO_BUTTON:
			{
				MouseButton button = map.from.mouseButton;
				UpdateButtonFromButton(map.to.button, IsMouseButtonDown(button), IsMouseButtonPressed(button), IsMouseButtonReleased(button));
			} break;
			case MOUSE_BUTTON_TO_AXIS:
			{
				MouseButton button = map.from.mouseButton;
				UpdateAxisFromButton(map.to.axis, 
					IsMouseButtonDown(button), 
					map.xWhenPressed, map.yWhenPressed);
			} break;
			case CONTROLLER_BUTTON_TO_BUTTON:
			{
				GamepadButton button = map.from.controllerButton;
				UpdateButtonFromButton(map.to.button, 
					IsGamepadButtonDown(0, button), 
					IsGamepadButtonPressed(0, button), 
					IsGamepadButtonReleased(0, button));
			} break;
			case CONTROLLER_BUTTON_TO_AXIS:
			{
				GamepadButton button = map.from.controllerButton;
				UpdateAxisFromButton(map.to.axis,
					IsGamepadButtonDown(0, button),
					map.xWhenPressed,
					map.yWhenPressed);
			} break;
			case CONTROLLER_AXIS_TO_BUTTON:
			{
				GamepadAxis axis = map.from.controllerAxis;
				float x = GetGamepadAxisMovement(0, axis + 0);
				float y = GetGamepadAxisMovement(0, axis + 1);
				UpdateButtonFromAxis(map.to.button, x, y, map.dotX, map.dotY, map.threshold);
			} break;
			case CONTROLLER_AXIS_TO_AXIS:
			{
				GamepadAxis axis = map.from.controllerAxis;
				float x = GetGamepadAxisMovement(0, axis + 0);
				float y = GetGamepadAxisMovement(0, axis + 1);
				UpdateAxisFromAxis(map.to.axis, x, y);
			} break;
		}
	}
}
