#include "Utilities.h"

std::map<std::string, sf::Sprite> Notes;

int main(int argc, char* argv[])
{
	std::ofstream file("game_log.log");
	TeeBuf teeBuf(std::cout.rdbuf(), file.rdbuf());
	std::streambuf* coutBuf = std::cout.rdbuf(&teeBuf);
	std::streambuf* cerrBuf = std::cerr.rdbuf(&teeBuf);
	BasePath = GetBashPath();
	std::cout << "[INFO] Basepath is located at: " << BasePath << "\n";
	Notes_NotActive = {
		{"Left_NotActive", BasePath + "/assets/_LeftNotActive.png"},
		{"Down_NotActive", BasePath + "/assets/_DownNotActive.png"},
		{"Up_NotActive", BasePath + "/assets/_UpNotActive.png"},
		{"Right_NotActive",BasePath + "/assets/_RightNotActive.png"},
	};
	Notes_Active = {
		{"Left_Active", BasePath + "/assets/_LeftActive.png"},
		{"Down_Active", BasePath + "/assets/_DownActive.png"},
		{"Up_Active",BasePath + "/assets/_UpActive.png"},
		{"Right_Active", BasePath + "/assets/_RightActive.png"},
		{"Left_ActiveBeenHolden", BasePath + "/assets/_LeftActive.png"},
		{"Down_ActiveBeenHolden", BasePath + "/assets/_DownActive.png"},
		{"Up_ActiveBeenHolden", BasePath + "/assets/_UpActive.png"},
		{"Right_ActiveBeenHolden", BasePath + "/assets/_RightActive.png"},
	};
	HoldPath = BasePath + "/assets/Hold.png";
	HoldPath_Start = BasePath + "/assets/Hold_Start.png";
	HoldPath_End = BasePath + "/assets/Hold_End.png";
	std::cout << "[INFO] Checking if windows...\n";
	if (IsWindows)
	{
		std::cout << "[INFO] Is Windows! \n";
		if (IsFirstLaunch())
		{
			std::cout << "[INFO] Players first launch! Adding some keys...\n";
			AddFirstTimeLaunchKey();
		}
	}
	CreateActivity("Playing song: Test song!", "Hopefully they are winning....", "test", "test", "test", "test");
	int height, width;
	height = sf::VideoMode::getDesktopMode().height;
	width = sf::VideoMode::getDesktopMode().width;
	sf::RenderWindow window(sf::VideoMode(width, height), "Project Rhythm", sf::Style::Fullscreen);
	sf::Font font;
	if (!font.loadFromFile(BasePath + "/font/Arial.ttf"))
	{
		std::cerr << "[ERROR] Failed to load font from path " << BasePath + "/font/Arial.ttf" << "\n";
		return EXIT_FAILURE;
	}
	std::cout << "[INFO] Loaded Font from path " << BasePath + "/font/Arial.ttf" << "\n";
	for (auto const& [key, val] : Notes_NotActive)
	{
		sf::Texture texture;
		if (!texture.loadFromFile(val))
		{
			std::cerr << "[ERROR] Failed to load texture from path " << val << "\n";
			return EXIT_FAILURE;
		}
		std::cout << "[INFO] Loaded texture from path " << val << "\n";
		Arrows_Textures[key] = texture;
	}
	for (auto const& [key, val] : Notes_Active)
	{
		sf::Texture texture;
		if (!texture.loadFromFile(val))
		{
			std::cerr << "[ERROR] Failed to load texture from path " << val << "\n";
			return EXIT_FAILURE;
		}
		std::cout << "[INFO] Loaded texture from path " << val << "\n";
		Arrows_Textures[key] = texture;
	}
	for (auto const& [key, val] : Arrows_Textures)
	{
		sf::Sprite sprite(val);
		Arrows[key] = sprite;
		std::cout << "[INFO] Added sprite " << key << "\n";
	}
	if (!blank.loadFromFile(BasePath + "/assets/blank.png"))
	{
		std::cerr << "[ERROR] Failed to load texture from path assets/blank.png \n";
		return EXIT_FAILURE;
	}
	TextButton button("Quit!", font, 24, sf::Vector2f(100, 100), 3);
	Arrows["Left_NotActive"].setPosition(ArrowPosition["Left"]);
	Arrows["Down_NotActive"].setPosition(ArrowPosition["Down"]);
	Arrows["Up_NotActive"].setPosition(ArrowPosition["Up"]);
	Arrows["Right_NotActive"].setPosition(ArrowPosition["Right"]);
	window.clear(sf::Color::Black);
	LoadBaseTextures();
	window.draw(Arrows["Left_NotActive"]);
	window.draw(Arrows["Down_NotActive"]);
	window.draw(Arrows["Up_NotActive"]);
	window.draw(Arrows["Right_NotActive"]);
	//std::cout << BasePath + "/songs/test.json" << "\n";
	LoadChart("/songs/test.json", window);
	window.display();
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				KeyDown(event.key.scancode, window);
				break;
			case sf::Event::KeyReleased:
				KeyUp(event.key.scancode, window);
				tick = 0;
				break;
			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					if (button.contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))) {
						window.close();
					}
				}
			}
		}
		Update();
		window.clear(sf::Color::White);
		button.draw(window);
		window.draw(Arrows["Left_NotActive"]);
		window.draw(Arrows["Down_NotActive"]);
		window.draw(Arrows["Up_NotActive"]);
		window.draw(Arrows["Right_NotActive"]);
		/*
			small hack here to fuck the player over if they try to keep holding one of the keybinds lol!
		*/
		for (auto& [key, value] : Arrows)
		{
			if (value.getTexture() == &Arrows_Textures["Left_Active"])
			{
				if (tick >= 1)
				{ 
					tick = 0;
					value.setTexture(Arrows_Textures["Left_ActiveBeenHolden"]);
				}
				else
				{
					tick += 0.01;
				}
			}
			if (value.getTexture() == &Arrows_Textures["Down_Active"])
			{
				if (tick >= 1)
				{
					tick = 0;
					value.setTexture(Arrows_Textures["Down_ActiveBeenHolden"]);
				}
				else
				{
					tick += 0.1;
				}
			}
			if (value.getTexture() == &Arrows_Textures["Up_Active"])
			{
				if (tick >= 1)
				{
					tick = 0;
					value.setTexture(Arrows_Textures["Up_ActiveBeenHolden"]);
				}
				else
				{
					tick += 0.01;
				}
			}
			if (value.getTexture() == &Arrows_Textures["Right_Active"])
			{
				if (tick >= 1)
				{
					tick = 0;
					value.setTexture(Arrows_Textures["Right_ActiveBeenHolden"]);
				}
				else
				{
					tick += 0.01;
				}
			}
		}
		Note_Update(window, 1.0f/60.0f);
		for (auto& [key, note] : TapNotes)
		{
			if (note.getGlobalBounds().intersects(Arrows["Left_NotActive"].getGlobalBounds()))
			{
				//std::cout <<  << "\n";
				if (Arrows["Left_NotActive"].getTexture() == note.getTexture())
				{
					note.setTexture(blank);
				}
				if (Arrows["Right_NotActive"].getTexture() == note.getTexture())
				{
					note.setTexture(blank);
				}
				if (Arrows["Up_NotActive"].getTexture() == note.getTexture())
				{
					note.setTexture(blank);
				}
				if (Arrows["Down_NotActive"].getTexture() == note.getTexture())
				{
					note.setTexture(blank);
				}
			}
		}
	}
	std::cout << "[INFO] Logging to file\n";
	return EXIT_SUCCESS;
}