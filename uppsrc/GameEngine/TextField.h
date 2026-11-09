#ifndef UPP_TEXTFIELD_H
#define UPP_TEXTFIELD_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <GameLib/GameLib.h>
#include <GameEngine/UISystem.h>

NAMESPACE_UPP

// TextField UI element for text input
class UITextField : public UIElement {
public:
    UITextField(const String& text = String());
    virtual ~UITextField() = default;

    // Set/get text
    void SetText(const String& text);
    const String& GetText() const { return text; }

    // Set text color
    void SetTextColor(Color color) { textColor = color; }
    Color GetTextColor() const { return textColor; }

    // Set font
    void SetTextFont(Font font) { textFont = font; }
    Font GetTextFont() const { return textFont; }

    // Set/get background and border
    void SetBackgroundColor(Color color) { backgroundColor = color; }
    void SetBorderColor(Color color) { borderColor = color; }
    Color GetBackgroundColor() const { return backgroundColor; }
    Color GetBorderColor() const { return borderColor; }

    // Get cursor position
    int GetCursorPosition() const { return cursorPos; }
    void SetCursorPosition(int pos) { cursorPos = min(pos, text.GetLength()); }

    // Set callback for when text is changed
    void SetTextChangedCallback(std::function<void(const String&)> callback) { textChangedCallback = callback; }

    void Render(Draw& draw, const Rect& viewport) override;
    bool HandleInput(int x, int y, dword keyflags) override;
    bool HandleMouseMove(int x, int y) override;
    bool HandleMouseDown(int x, int y, int button) override;

    // Handle character input (for text entry)
    void InsertCharacter(char c);
    void DeleteCharacter();
    void BackspaceCharacter();

private:
    String text;
    int cursorPos = 0;
    bool hasFocus = false;
    bool showCursor = true;
    uint64 cursorBlinkStart = 0;
    static const int CURSOR_BLINK_TIME = 500; // milliseconds

    Color textColor = Color(0, 0, 0);         // Black text by default
    Color backgroundColor = Color(255, 255, 255);  // White background
    Color borderColor = Color(100, 100, 100);      // Gray border

    Font textFont = StdFont(12);

    // Callbacks
    std::function<void(const String&)> textChangedCallback;
};

END_UPP_NAMESPACE

#endif