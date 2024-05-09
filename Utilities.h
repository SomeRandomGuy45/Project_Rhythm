/*
Main handler of the game!!!
*/

#include <iostream>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <map>
#include <set>
#include <fstream>
#include <string>
#include <iostream>
#include <thread>
#include <discord-game-sdk/discord.h>
#include <vector>
#include <utility> 
#include <future>
#include <stdio.h>	
#include <time.h>
#include <sol/sol.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "Enums.h"
#include "json.hpp"
#if defined(_WIN32) || defined(WIN32)
#include<Windows.h>
bool IsWindows = true;
#else
bool IsWindows = false;
#endif

struct SpriteCompare //HOT FIX
{
	bool operator() (const sf::Sprite& lhs, const sf::Sprite& rhs) const
	{
		return &lhs.getPosition().y < &rhs.getPosition().y;
	}
};

std::string HoldPath = "assets/Hold.png";
sf::Texture HoldTexture;
std::string HoldPath_Start = "/assets/Hold_Start.png";
sf::Texture HoldTexture_Start;
std::string HoldPath_End = "/assets/Hold_End.png";
sf::Texture HoldTexture_End;
sf::Music gameSound;
using json = nlohmann::json;

std::string BasePath;

PlayState PlayerState = PLAYING;

std::ifstream GameConfigData(BasePath + "/ClientSettings/GameConfig.json");

std::map<std::string, std::string> Notes_NotActive = {
	{"Left_NotActive", BasePath + "/assets/_LeftNotActive.png"},
	{"Down_NotActive", BasePath + "/assets/_DownNotActive.png"},
	{"Up_NotActive", BasePath + "/assets/_UpNotActive.png"},
	{"Right_NotActive",BasePath + "/assets/_RightNotActive.png"},
};
std::map<std::string, std::string> Notes_Active = {
	{"Left_Active", BasePath + "/assets/_LeftActive.png"},
	{"Down_Active", BasePath + "/assets/_DownActive.png"},
	{"Up_Active",BasePath + "/assets/_UpActive.png"},
	{"Right_Active", BasePath + "/assets/_RightActive.png"},
	{"Left_ActiveBeenHolden", BasePath + "/assets/_LeftActive.png"},
	{"Down_ActiveBeenHolden", BasePath + "/assets/_DownActive.png"},
	{"Up_ActiveBeenHolden", BasePath + "/assets/_UpActive.png"},
	{"Right_ActiveBeenHolden", BasePath + "/assets/_RightActive.png"},
};
std::map<std::string, sf::Vector2f> ArrowPosition = {
	{"Left", sf::Vector2f(750, 900)},
	{"Down", sf::Vector2f(850, 900)},
	{"Up", sf::Vector2f(950, 900)},
	{"Right", sf::Vector2f(1050, 900)}
};
sf::Texture blank;
std::map<std::string, sf::Texture> Arrows_Textures;
std::map<std::string, sf::Sprite> Arrows;
std::map<sf::Sprite, bool, SpriteCompare> IsGettingHold;
std::map<std::string, std::string> Flags;
std::map<std::string, int> Keybinds = {
	{"Left", 0},
	{"Down", 18},
	{"Up", 22},
	{"Right", 3}
};
std::string keyDown;
std::set<int> pressedKeys;
sf::Vector2f resetPos = sf::Vector2f(10000, 10000);
discord::Core* core{};
std::map<sf::Sprite, sf::Sprite, SpriteCompare> HoldNotes;
std::map<sf::Sprite, sf::Sprite, SpriteCompare> TapNotes;
double tick = 0;
int BPM = 0;
bool RPC = true;

class TextButton {
public:
	TextButton(const std::string& text, const sf::Font& font, unsigned int characterSize, const sf::Vector2f& position, int outlineThickness)
		: m_text(text, font, characterSize), m_position(position) {
		// Set button properties
		m_text.setFillColor(sf::Color::Black); // Text color
		m_text.setPosition(position);
		// Calculate button size based on text size
		sf::FloatRect textBounds = m_text.getLocalBounds();
		m_button.setSize(sf::Vector2f(textBounds.width + 20, textBounds.height + 10)); // Adjust for padding
		m_button.setFillColor(sf::Color::White); // Button color
		m_button.setPosition(position);
		m_button.setOutlineThickness(outlineThickness); // Button outline thickness
		m_button.setOutlineColor(sf::Color::Black); // Button outline color
	}

	void draw(sf::RenderWindow& window) const {
		window.draw(m_button);
		window.draw(m_text);
	}

	bool contains(const sf::Vector2f& point) const {
		return m_button.getGlobalBounds().contains(point);
	}

private:
	sf::Text m_text;
	sf::RectangleShape m_button;
	sf::Vector2f m_position;
};

struct Section
{
	std::string y;
	std::string arrow;
	std::string IsHold;
	std::string Length;
};

struct ChartData
{
	struct Song
	{
		std::vector<struct Section> notes;
	} song;
};

const std::string currentDateTime() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	if (localtime_s(&tstruct, &now) != 0) {
		// Error handling if localtime_s fails
		return "[ERROR] Failed to get local time";
	}
	if (strftime(buf, sizeof(buf), "%Y-%m-%d-%H-%M-%S", &tstruct) == 0) {
		// Error handling if strftime fails
		return "[ERROR] Failed to format time";
	}
	return buf;
}

void TestLua()
{
	std::cout << "[INFO] Testing lua...\n";
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::package);
	lua.script("print('[INFO] From Lua script and this works!')");
}

std::vector<std::variant<std::string, int, bool>> ConvertArgs(const std::vector<std::string>& args)
{
	std::vector<std::variant<std::string, int, bool>> convertedArgs;
	for (const auto& arg : args)
	{
		convertedArgs.emplace_back(arg);
	}
	return convertedArgs;
}

void RunLuaFile(std::string path, bool should_args, std::vector<std::string>& args)
{
	//what the heck
	std::cout << "[INFO] Running lua script with path " << path << "\n";
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::package);
	lua.require_file("api", BasePath + "/api/api.lua");
	lua.require_file("table_plus", BasePath + "/api/table_plus.lua");
	sol::load_result script = lua.load_file(path);
	sol::table luaArgs = lua.create_table();
	for (size_t i = 0; i < args.size(); ++i)
	{
		luaArgs.add(args[i]); //tf!
	}

	if (should_args)
	{
		script(luaArgs);
	}
	else
	{
		script();
	}
}

/*
	this class does... something....
	fucking hack idk
*/
class TeeBuf : public std::streambuf {

public:
	TeeBuf(std::streambuf* buf1, std::streambuf* buf2) : buf1(buf1), buf2(buf2) {}

protected:
	virtual int overflow(int c) {
		if (c != EOF) {
			if (buf1 && buf2) {
				buf1->sputc(c);
				buf2->sputc(c);
			}
		}
		return c;
	}

	virtual int sync() {
		if (buf1) buf1->pubsync();
		if (buf2) buf2->pubsync();
		return 0;
	}

private:
	std::streambuf* buf1;
	std::streambuf* buf2;
};

bool solve(std::string ip) {
	bool op;
	std::istringstream(ip) >> std::boolalpha >> op;
	return op;
}

std::string GetBashPath()
{
	TCHAR buffer[MAX_PATH];
	GetModuleFileName(nullptr, buffer, MAX_PATH);
	std::wstring fullPath(buffer);
	// Find the position of the last backslash to get the directory
	size_t pos = fullPath.find_last_of(L"\\/");
	if (pos != std::wstring::npos) {
		return std::string(fullPath.begin(), fullPath.begin() + pos);
	}
	return "";
}

void HideConsole()
{
#if defined(WIN32) || defined(_WIN32)
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif
}

void ShowConsole()
{
#if defined(WIN32) || defined(_WIN32)
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
#endif
}

bool IsConsoleVisible()
{
#if defined(WIN32) || defined(_WIN32)
	return ::IsWindowVisible(::GetConsoleWindow()) != FALSE;
#endif
}

bool IsFirstLaunch()
{
#if defined(WIN32) || defined(_WIN32)
	HKEY hKey;
	LPCWSTR keyPath = L"SOFTWARE\\SomeRandomGuyzGames\\ProjectRhythm";
	LPCWSTR valueName = L"FirstTimeLaunch";
	DWORD valueType;
	DWORD valueData;
	DWORD valueSize = sizeof(DWORD);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return true;
	}

	// Try to read the value
	if (RegQueryValueEx(hKey, valueName, NULL, &valueType, (LPBYTE)&valueData, &valueSize) != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return true;
	}
	RegCloseKey(hKey);
	return false;
#endif
}

void Register(std::string key, std::string name, std::string handler)
{
#if defined(WIN32) || defined(_WIN32)
	std::string handlerArgs = "\"" + handler + "\" %1";

	HKEY uriKey, uriIconKey, uriCommandKey;
	RegCreateKeyExA(HKEY_CURRENT_USER, ("Software\\Classes\\" + key).c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &uriKey, nullptr);
	RegCreateKeyExA(uriKey, "DefaultIcon", 0, nullptr, 0, KEY_WRITE, nullptr, &uriIconKey, nullptr);
	RegCreateKeyExA(uriKey, "shell\\open\\command", 0, nullptr, 0, KEY_WRITE, nullptr, &uriCommandKey, nullptr);

	std::string value;
	DWORD size = MAX_PATH;

	RegGetValueA(uriKey, "", nullptr, RRF_RT_REG_SZ, nullptr, &value[0], &size);
	if (value.empty()) {
		RegSetValueExA(uriKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(("URL: " + name + " Protocol").c_str()), sizeof("URL: " + name + " Protocol"));
		RegSetValueExA(uriKey, "URL Protocol", 0, REG_SZ, reinterpret_cast<const BYTE*>(""), sizeof(""));
	}

	RegGetValueA(uriCommandKey, "", nullptr, RRF_RT_REG_SZ, nullptr, &value[0], &size);
	if (value != handlerArgs) {
		RegSetValueExA(uriIconKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(handler.c_str()), sizeof(handler));
		RegSetValueExA(uriCommandKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(handlerArgs.c_str()), sizeof(handlerArgs));
	}

	RegCloseKey(uriKey);
	RegCloseKey(uriIconKey);
	RegCloseKey(uriCommandKey);
#endif
}

void AddFirstTimeLaunchKey()
{
#if defined(WIN32) || defined(_WIN32)
	HKEY hKey;
	LPCWSTR keyPath = L"SOFTWARE\\SomeRandomGuyzGames\\ProjectRhythm";
	LPCWSTR valueName = L"FirstTimeLaunch";
	DWORD valueData = 1;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, keyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
		std::cout << "[INFO] Added Key!\n";
		RegSetValueEx(hKey, valueName, 0, REG_DWORD, (BYTE*)&valueData, sizeof(DWORD));
		RegCloseKey(hKey);
	}
	//Register("project-rhythm-test2", "Project-Rhythm", BasePath + "/Project_Rhythm.exe");
#endif
}

void LoadBaseTextures()
{
	if (!HoldTexture.loadFromFile(HoldPath))
	{
		std::cerr << "[ERROR] Failed to load texture from path " << HoldPath << "\n";
		return;
	}
	std::cout << "[INFO] Loaded texture from path " << HoldPath << "\n";
	if (!HoldTexture_Start.loadFromFile(HoldPath_Start))
	{
		std::cerr << "[ERROR] Failed to load texture from path " << HoldPath_Start << "\n";
		return;
	}
	std::cout << "[INFO] Loaded texture from path " << HoldPath_Start << "\n";
	std::cout << "[INFO] Loaded texture from path " << HoldPath << "\n";
	if (!HoldTexture_End.loadFromFile(HoldPath_End))
	{
		std::cerr << "[ERROR] Failed to load texture from path " << HoldPath_End << "\n";
		return;
	}
	std::cout << "[INFO] Loaded texture from path " << HoldPath_End << "\n";
}

void CreateNote(std::string Arrow, std::string Y, std::string IsHold, std::string SIZE ,sf::RenderWindow& window)
{
	bool isHold = solve(IsHold);
	float y = std::stof(Y);
	float size = std::stof(SIZE);
	if (Arrow != "")
	{
		if (isHold)
		{
			if (Arrow == "left")
			{
				float NEWSIZE_Y = 64 * size;
				sf::Vector2f targetSize(64, NEWSIZE_Y);
				sf::Sprite note(HoldTexture);
				note.setPosition(750, y);
				note.setScale(
					targetSize.x / note.getLocalBounds().width,
					targetSize.y / note.getLocalBounds().height);
				HoldNotes[note] = note;
				window.draw(note);
				window.display();
			}
			else if (Arrow == "down")
			{
				float NEWSIZE_Y = 64 * size;
				sf::Vector2f targetSize(64, NEWSIZE_Y);
				sf::Sprite note(HoldTexture);
				note.setPosition(850, y);
				note.setScale(
					targetSize.x / note.getLocalBounds().width,
					targetSize.y / note.getLocalBounds().height);
				HoldNotes[note] = note;
				window.draw(note);
				window.display();
			}
			else if (Arrow == "up")
			{
				float NEWSIZE_Y = 64 * size;
				sf::Vector2f targetSize(64, NEWSIZE_Y);
				sf::Sprite note(HoldTexture);
				note.setPosition(950, y);
				note.setScale(
					targetSize.x / note.getLocalBounds().width,
					targetSize.y / note.getLocalBounds().height);
				HoldNotes[note] = note;
				window.draw(note);
				window.display();
			}
			else if (Arrow == "right")
			{
				float NEWSIZE_Y = 64 * size;
				sf::Vector2f targetSize(64, NEWSIZE_Y);
				sf::Sprite note(HoldTexture);
				note.setPosition(1050, y);
				note.setScale(
					targetSize.x / note.getLocalBounds().width,
					targetSize.y / note.getLocalBounds().height);
				HoldNotes[note] = note;
				window.draw(note);
				window.display();
			}
		}
		else
		{
			if (Arrow == "left")
			{
				sf::Sprite note(Arrows_Textures["Left_Active"]);
				note.setPosition(750, y);
				TapNotes[note] = note;
				window.draw(note);
				window.display();
			}
			else if (Arrow == "down")
			{
				sf::Sprite note(Arrows_Textures["Down_Active"]);
				note.setPosition(850, y);
				TapNotes[note] = note;
				window.draw(note);
				window.display();
			}
			else if(Arrow == "up")
			{
				sf::Sprite note(Arrows_Textures["Up_Active"]);
				note.setPosition(950, y);
				TapNotes[note] = note;
				window.draw(note);
				window.display();
			}
			else if (Arrow == "right")
			{
				sf::Sprite note(Arrows_Textures["Right_Active"]);
				note.setPosition(1050, y);
				TapNotes[note] = note;
				window.draw(note);
				window.display();
			}
		}
	}
}

void Note_Update(sf::RenderWindow& window, double deltaTime) //TODO: speed
{
	if (BPM == NULL)
	{
		for (auto& [key, note] : HoldNotes)
		{
			float NewY = note.getPosition().y + 0.01f;
			float x = note.getPosition().x;
			note.setPosition(x, NewY);
			window.draw(note);
		}
		for (auto& [key, note] : TapNotes)
		{
			float NewY = note.getPosition().y + 0.01f;
			float x = note.getPosition().x;
			note.setPosition(x, NewY);
			window.draw(note);
		}
	}
	else
	{
		double speed = ((deltaTime * BPM) / 45);
		for (auto& [key, note] : HoldNotes)
		{
			float NewY = note.getPosition().y + speed;
			float x = note.getPosition().x;
			note.setPosition(x, NewY);
			window.draw(note);
		}
		for (auto& [key, note] : TapNotes)
		{
			float NewY = note.getPosition().y + speed;
			float x = note.getPosition().x;
			note.setPosition(x, NewY);
			window.draw(note);
		}
	}
	window.display();
}

json GetFlags()
{
	if (!GameConfigData.is_open())
	{
		std::cout << "[ERROR] Couldn't import GameConfig.json!\n";
		return "";
	}
	json Data = json::parse(GameConfigData);
	return Data;
}
void CreateActivity(std::string State, std::string Details, std::string LargeImage, std::string SmallImage, std::string LargeText, std::string SmallText)
{
	if (RPC)
	{
		if (SmallImage == "")
		{
			SmallImage = LargeImage;
		}
		std::cout << "[INFO] Setting up new Activity/RPC with info, State: " << State << ", Details: " << Details << ", Large Image: " << LargeImage << ", Small Image: " << SmallImage << ", Large text: " << LargeText << ", and Small text: " << SmallText << "\n";
		auto result = discord::Core::Create(1236762772549927045, DiscordCreateFlags_Default, &core);
		discord::Activity activity{};
		activity.SetState(State.c_str());
		activity.SetDetails(Details.c_str());
		activity.SetType(discord::ActivityType::Playing);
		activity.GetAssets().SetLargeImage(LargeImage.c_str());
		activity.GetAssets().SetSmallImage(SmallImage.c_str());
		activity.SetInstance(true);
		if (LargeText != "")
		{
			activity.GetAssets().SetLargeText(LargeText.c_str());
		}
		if (SmallImage != "")
		{
			activity.GetAssets().SetSmallText(SmallText.c_str());
		}
		core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
			if (result == discord::Result::Ok)
			{
				std::cout << "[INFO] Successfully updated Activity/RPC!\n";
			}
			else
			{
				std::cerr << "[ERROR] Failed to update Activity/RPC!\n";
			}
		}); //hellish line
	}
}

void Update()
{
	::core->RunCallbacks(); //wtf is this line LOL
}

void KeyDown(int KeyCode, sf::RenderWindow& window)
{
	pressedKeys.insert(KeyCode);
	if (PlayerState == PLAYING)
	{
		for (auto it = Keybinds.begin(); it != Keybinds.end(); ++it)
		{
			if (it->second == KeyCode)
			{
				keyDown = it->first;
				if (keyDown == "Left")
				{
					Arrows["Left_NotActive"].setTexture(Arrows_Textures["Left_Active"]);
				}
				else if (keyDown == "Right")
				{
					Arrows["Right_NotActive"].setTexture(Arrows_Textures["Right_Active"]);
				}
				else if (keyDown == "Up")
				{
					Arrows["Up_NotActive"].setTexture(Arrows_Textures["Up_Active"]);
				}
				else if (keyDown == "Down")
				{
					Arrows["Down_NotActive"].setTexture(Arrows_Textures["Down_Active"]);
				}
			}
		}
	}
}

void KeyUp(int KeyCode, sf::RenderWindow& window)
{
	pressedKeys.erase(KeyCode);
	for (auto it = Keybinds.begin(); it != Keybinds.end(); ++it)
	{
		if (pressedKeys.find(it->second) == pressedKeys.end())
		{
			if (it->first == "Left")
			{
				Arrows["Left_NotActive"].setTexture(Arrows_Textures["Left_NotActive"]);
			}
			if (it->first == "Right")
			{
				Arrows["Right_NotActive"].setTexture(Arrows_Textures["Right_NotActive"]);
			}
			if (it->first == "Up")
			{
				Arrows["Up_NotActive"].setTexture(Arrows_Textures["Up_NotActive"]);
			}
			if (it->first == "Down")
			{
				Arrows["Down_NotActive"].setTexture(Arrows_Textures["Down_NotActive"]);
			}
		}
	}
}

void RunChart(ChartData chartData, sf::RenderWindow& window)
{
	unsigned int Chart_Notes_Length = chartData.song.notes.size();
	//draws motherfuckin notes!
	for (unsigned int i = 0; i < Chart_Notes_Length; i++)
	{
		std::string arrow_ig = chartData.song.notes[i].arrow;
		std::string arrow_y = chartData.song.notes[i].y;
		std::string arrow_hold = chartData.song.notes[i].IsHold;
		std::string length = chartData.song.notes[i].Length;
		CreateNote(arrow_ig, arrow_y, arrow_hold, length,window);
	}
}

void LoadSong(std::string Path)
{
	std::string Song_Path = BasePath + Path;
	if (!gameSound.openFromFile(Song_Path))
	{
		std::cerr << "[ERROR] Unable to load song with play " << Song_Path << "\n";
		return;
	}
	std::cout << "[INFO] Loaded song with path " << Song_Path << "\n";
	gameSound.play();
}

void LoadChart(std::string Path, sf::RenderWindow& window) //initchart!
{
	//took to long to fucking write LOL
	std::cout << BasePath + Path << "\n";
	ChartData chartData;
	std::fstream Song(BasePath + Path);
	if (!Song.is_open())
	{
		std::cerr << "[ERROR] Unable to open chart\n";
		return;
	}
	std::cout << "[INFO] Opened json\n";
	json Song_Data = json::parse(Song);
	std::cout << "[INFO] Parse json!\n";
	if (Song_Data["bpm"].is_null()) //what if no bpm
	{
		BPM = 100;
	}
	else
	{
		std::string bpm_str = Song_Data["bpm"];
		BPM = std::stoi(bpm_str);
	}
	LoadSong(Song_Data["song_path"]);
	std::cout << "[INFO] Chart BPM is " << BPM << "\n";
	auto& notes = Song_Data["notes"];
	std::cout << "[INFO] Got chart notes\n";
	int i = 0;
	for (auto &note : notes)
	{
		Section section;
		section.y = note.value("y", "0"); // Set default Y value to "0" if not present
		section.arrow = note.value("arrow", "left"); // Set default arrow value to "left" if not present
		section.IsHold = note.value("isHold", "false"); // Set default isHold value to "false" if not present
		section.Length = note.value("length", "2");
		chartData.song.notes.push_back(section);
		++i;
	}
	RunChart(chartData, window);
}