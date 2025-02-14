// Copyright (c) 2018, Jason Justian
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

class TrigSeq16 : public HemisphereApplet {
public:

    const char* applet_name() { // Maximum 10 characters
        return "Trig16";
    }
    const uint8_t* applet_icon() { return PhzIcons::trigseq16; }

    void Start() {
        ForEachChannel(ch)
        {
            pattern[ch] = random(1, 255);
        }
        step = 0;
        end_step = 15;
        cursor = 0;
    }

    void Reset() {
        step = -1;
    }

    void Controller() {
        if (Clock(1)) Reset();

        if (Clock(0)) {
            bool swap = In(0) >= HEMISPHERE_3V_CV;
            if (step >= end_step) step = -1;
            step++;
            active_step = Step(); // actual step after Offset modulation
            if (active_step < 8) {
                if ((pattern[0] >> active_step) & 0x01) ClockOut(swap ? 1 : 0);
                else ClockOut(swap ? 0 : 1);
            } else {
                if ((pattern[1] >> (active_step - 8)) & 0x01) ClockOut(swap ? 1 : 0);
                else ClockOut(swap ? 0 : 1);
            }
        }
    }

    void View() {
        DrawDisplay();
    }

    void OnButtonPress() {
        cursor++;
        if (cursor > 4) cursor = 0;
        ResetCursor();
    }

    void OnEncoderMove(int direction) {
        // Update end_step
        if (cursor == 4) {
            end_step = constrain(end_step + direction, 0, 15);
        } else {
            int ch = cursor > 1 ? 1 : 0;
            int this_cursor = cursor - (ch * 2);

            // Get the current pattern
            int curr_patt = pattern[ch];

            // Shift right based on which nybble is selcted
            uint8_t nybble = (curr_patt >> (this_cursor * 4)) & 0x0f;
            nybble += direction;
            nybble &= 0x0f;

            // Put the updated nybble back where it belongs
            if (this_cursor == 0) pattern[ch] = (curr_patt & 0xf0) + nybble;
            else pattern[ch] = (curr_patt & 0x0f) + (nybble << 4);
        }
    }

    uint64_t OnDataRequest() {
        uint64_t data = 0;
        Pack(data, PackLocation {0,8}, pattern[0]);
        Pack(data, PackLocation {8,8}, pattern[1]);
        Pack(data, PackLocation {16,4}, end_step);
        return data;
    }

    void OnDataReceive(uint64_t data) {
        pattern[0] = Unpack(data, PackLocation {0,8});
        pattern[1] = Unpack(data, PackLocation {8,8});
        end_step = Unpack(data, PackLocation {16,4});

        step = -1;
    }

protected:
    void SetHelp() {
        //                    "-------" <-- Label size guide
        help[HELP_DIGITAL1] = "Clock";
        help[HELP_DIGITAL2] = "Reset";
        help[HELP_CV1]      = "Swap Ch";
        help[HELP_CV2]      = "Offset";
        help[HELP_OUT1]     = "Trg";
        help[HELP_OUT2]     = "Inverse";
        help[HELP_EXTRA1] = "";
        help[HELP_EXTRA2] = "";
       //                   "---------------------" <-- Extra text size guide
    }

private:
    int step; // Current step
    int active_step;
    uint8_t pattern[2];
    int end_step;
    int cursor; // 0=ch1 low, 1=ch1 hi, 2=ch2 low, 3=ch3 hi, 4=end_step

    int Offset() {
        int offset = Proportion(DetentedIn(1), HEMISPHERE_MAX_INPUT_CV, end_step);
        if (offset < 0) offset += Length();
        offset %= Length();
        return offset;
    }

    inline int Length() const {
        return end_step + 1;
    }

    int Step() {
        int s = step + Offset();
        s %= Length();
        return s;
    }

    void DrawDisplay() {
        bool stop = 0; // Stop displaying when end_step is reached

        int offset = Offset();

        ForEachChannel(ch)
        {
            int x = 10 + (31 * ch);

            // Draw the steps for this channel
            for (int s = 0; s < 8; s++)
            {
                if (!stop) {
                    int y = 18 + (6 * s);
                    gfxCircle(x, y, 3);
                    int value = (pattern[ch] >> s) & 0x01;
                    if (value) {
                        for (int r = 1; r < 3; r++) {
                            gfxCircle(x, y, r);
                            gfxCircle(x + 1, y, r);
                            gfxCircle(x + 2, y, r);
                        }
                    }

                    if (s + (ch * 8) == active_step) {
                        gfxLine(x + 4, y, x + 10, y);
                    }

                    if (s + (ch * 8) == offset) {
                        gfxFrame(x - 4, y - 4, 9, 9);
                    }

                    // Draw the end_step cursor
                    if (s + (ch * 8) == end_step) {
                        if (cursor == 4) {
                            gfxLine(x - 8, y + 3, x + 5, y + 3);
                        }
                        stop = 1;
                    }
                }
            }

            // Draw the nybble cursor
            if (cursor < 4) {
                if (ch == (cursor > 1 ? 1 : 0) && CursorBlink()) {
                    int this_cursor = cursor - (ch * 2);
                    int y = 15 + (this_cursor * 24);
                    gfxLine(x - 5, y, x - 5, y + 24);
                }
            }
        }
    }
};
