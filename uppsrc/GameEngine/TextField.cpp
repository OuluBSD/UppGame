#include "TextField.h"

NAMESPACE_UPP

UITextField::UITextField(const String& text) : text(text) {
    // Set default size based on font and some reasonable text length
    Size text_size = GetTextSize("ABCDEFGHIJKLMNOPQRSTUVWXYZ", textFont);
    size = Size(text_size.cx + 20, text_size.cy + 10); // Add padding
    cursorPos = text.GetLength();
}

void UITextField::SetText(const String& newText) {
    text = newText;
    cursorPos = text.GetLength();
    
    if (textChangedCallback) {
        textChangedCallback(text);
    }
}

void UITextField::Render(Draw& draw, const Rect& viewport) {
    if (!visible || !enabled) return;

    // Draw background
    draw.DrawRect(Rect(pos, size), backgroundColor);

    // Draw border
    draw.DrawRect(Rect(pos, size), 1, borderColor);

    // Draw text
    draw.DrawText(pos.x + 5, pos.y + (size.cy - textFont.GetHeight()) / 2, text, textFont, textColor);

    // Draw cursor if focused and should be visible
    if (hasFocus && showCursor) {
        // Calculate cursor position
        String textBeforeCursor = text.Mid(0, cursorPos);
        Size textBeforeCursorSize = GetTextSize(textBeforeCursor, textFont);
        int cursorX = pos.x + 5 + textBeforeCursorSize.cx; // Account for left padding
        int cursorY = pos.y + (size.cy - textFont.GetHeight()) / 2;
        
        draw.DrawRect(cursorX, cursorY, 1, textFont.GetHeight(), textColor);
    }
}

bool UITextField::HandleInput(int x, int y, dword keyflags) {
    if (!visible || !enabled || !hasFocus) return false;

    // Handle various key inputs
    if (keyflags & K_KEYUP) {
        // Handle key release
        return true;
    }

    if (keyflags & K_ALT) {
        // Handle Alt key combinations
        return true;
    }

    if (keyflags & K_CTRL) {
        // Handle Ctrl key combinations
        if (keyflags & K_LEFT) {
            // Move cursor to beginning of text
            cursorPos = 0;
            return true;
        } else if (keyflags & K_RIGHT) {
            // Move cursor to end of text
            cursorPos = text.GetLength();
            return true;
        } else if (keyflags & K_HOME) {
            cursorPos = 0;
            return true;
        } else if (keyflags & K_END) {
            cursorPos = text.GetLength();
            return true;
        } else if (keyflags & K_BACKSPACE) {
            // Ctrl+Backspace - delete word
            while (cursorPos > 0 && text[cursorPos - 1] == ' ') {
                cursorPos--;
            }
            while (cursorPos > 0 && text[cursorPos - 1] != ' ' && text[cursorPos - 1] != '\t') {
                text.Remove(cursorPos - 1, 1);
                cursorPos--;
            }
            if (textChangedCallback) {
                textChangedCallback(text);
            }
            return true;
        }
    }

    // Handle regular keys
    if (keyflags & K_LEFT) {
        if (cursorPos > 0) {
            cursorPos--;
        }
        return true;
    } else if (keyflags & K_RIGHT) {
        if (cursorPos < text.GetLength()) {
            cursorPos++;
        }
        return true;
    } else if (keyflags & K_HOME) {
        cursorPos = 0;
        return true;
    } else if (keyflags & K_END) {
        cursorPos = text.GetLength();
        return true;
    } else if (keyflags & K_BACKSPACE) {
        BackspaceCharacter();
        return true;
    } else if (keyflags & K_DELETE) {
        DeleteCharacter();
        return true;
    } else if (keyflags & K_ENTER) {
        // Text field doesn't typically handle enter in single line mode
        // But we could call a callback here if we wanted multi-line behavior
        return true;
    } else if (keyflags & K_ESCAPE) {
        // Cancel editing?
        return true;
    }

    // Special handling for character keys
    dword chr = keyflags & 0xFFFF;
    if (chr >= 32 && chr <= 126) {  // Printable ASCII
        if (chr == '`' && (keyflags & K_CTRL)) {
            // Skip control characters that are also printable
            return false;
        }
        InsertCharacter((char)chr);
        return true;
    }

    return false;
}

bool UITextField::HandleMouseMove(int x, int y) {
    if (!visible || !enabled) return false;
    return false;
}

bool UITextField::HandleMouseDown(int x, int y, int button) {
    if (!visible || !enabled) return false;

    if (button == 0 && Contains(x, y)) {  // Left mouse button
        // Calculate approximate character position based on click position
        int relativeX = x - (pos.x + 5); // Account for padding
        int charIndex = 0;
        
        // Simple approximation for cursor position
        for (int i = 0; i <= text.GetLength(); i++) {
            String testStr = text.Mid(0, i);
            Size testSize = GetTextSize(testStr, textFont);
            
            // If we've gone past the click position, use this index
            if (testSize.cx > relativeX) {
                charIndex = i > 0 ? i - 1 : 0;
                break;
            } else if (i == text.GetLength()) {
                charIndex = i; // Clicked after the end of text
                break;
            }
        }
        
        cursorPos = charIndex;
        hasFocus = true;
        showCursor = true;
        cursorBlinkStart = GetTickCount();
        return true;
    }
    
    if (button == 0 && !Contains(x, y) && hasFocus) {
        // Clicked outside, lose focus
        hasFocus = false;
        return false;
    }
    
    return button == 0;
}

void UITextField::InsertCharacter(char c) {
    if (hasFocus) {
        text.Insert(cursorPos, String(c));
        cursorPos++;
        
        if (textChangedCallback) {
            textChangedCallback(text);
        }
    }
}

void UITextField::DeleteCharacter() {
    if (hasFocus && cursorPos < text.GetLength()) {
        text.Remove(cursorPos, 1);
        
        if (textChangedCallback) {
            textChangedCallback(text);
        }
    }
}

void UITextField::BackspaceCharacter() {
    if (hasFocus && cursorPos > 0) {
        text.Remove(cursorPos - 1, 1);
        cursorPos--;
        
        if (textChangedCallback) {
            textChangedCallback(text);
        }
    }
}

END_UPP_NAMESPACE