#ifndef MINILIST_HPP
#define MINILIST_HPP

#include <tesla.hpp>

namespace tsl {
    namespace elm {

        u32 ListItemDefaultHeight = 40;

        /**
         * @brief A item that goes into a list
         *
         */
        class CustomMiniListItem : public Element {
        public:
            /**
             * @brief Constructor
             *
             * @param text Initial description text
             */
            CustomMiniListItem(const std::string& text, const std::string& value = "")
                : Element(), m_text(text), m_value(value), m_highlightShakingStartTime(0) {
            }
            virtual ~CustomMiniListItem() {}

            virtual void draw(gfx::Renderer *renderer) override {
                if (this->m_touched && Element::getInputMode() == InputMode::Touch) {
                    renderer->drawRect(ELEMENT_BOUNDS(this), a(tsl::style::color::ColorClickAnimation));
                }

                if (this->m_maxWidth == 0) {
                    if (this->m_value.length() > 0) {
                        auto [valueWidth, valueHeight] = renderer->drawString(this->m_value.c_str(), false, 0, 0, 20, tsl::style::color::ColorTransparent);
                        this->m_maxWidth = this->getWidth() - valueWidth - 70;
                    } else {
                        this->m_maxWidth = this->getWidth() - 40;
                    }

                    auto [width, height] = renderer->drawString(this->m_text.c_str(), false, 0, 0, 23, tsl::style::color::ColorTransparent);
                    this->m_trunctuated = static_cast<u32>(width) > this->m_maxWidth;

                    if (this->m_trunctuated) {
                        this->m_scrollText = this->m_text + "        ";
                        auto [scrollWidth, scrollHeight] = renderer->drawString(this->m_scrollText.c_str(), false, 0, 0, 23, tsl::style::color::ColorTransparent);
                        this->m_scrollText += this->m_text;
                        this->m_textWidth = scrollWidth;
                        this->m_ellipsisText = renderer->limitStringLength(this->m_text, false, 22, this->m_maxWidth);
                    } else {
                        this->m_textWidth = width;
                    }
                }

                renderer->drawRect(this->getX(), this->getY(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));
                renderer->drawRect(this->getX(), this->getTopBound(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));

                if (this->m_trunctuated) {
                    if (this->m_focused) {
                        renderer->enableScissoring(this->getX(), this->getY(), this->m_maxWidth + 40, this->getHeight());
                        renderer->drawString(this->m_scrollText.c_str(), false, this->getX() + 20 - this->m_scrollOffset, this->getY() + 27, 23, tsl::style::color::ColorText);
                        renderer->disableScissoring();
                        if (this->m_scrollAnimationCounter == 90) {
                            if (this->m_scrollOffset == this->m_textWidth) {
                                this->m_scrollOffset = 0;
                                this->m_scrollAnimationCounter = 0;
                            } else {
                                this->m_scrollOffset++;
                            }
                        } else {
                            this->m_scrollAnimationCounter++;
                        }
                    } else {
                        renderer->drawString(this->m_ellipsisText.c_str(), false, this->getX() + 20, this->getY() + 27, 23, a(tsl::style::color::ColorText));
                    }
                } else {
                    renderer->drawString(this->m_text.c_str(), false, this->getX() + 20, this->getY() + 27, 23, a(tsl::style::color::ColorText));
                }

                renderer->drawString(this->m_value.c_str(), false, this->getX() + this->m_maxWidth + 45, this->getY() + 27, 20, this->m_faint ? a(tsl::style::color::ColorDescription) : a(tsl::style::color::ColorHighlight));
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                this->setBoundaries(this->getX(), this->getY(), this->getWidth(), ListItemDefaultHeight);
            }

            virtual bool onClick(u64 keys) override {
                if (keys & HidNpadButton_A)
                    this->triggerClickAnimation();
                else if (keys & (HidNpadButton_AnyUp | HidNpadButton_AnyDown | HidNpadButton_AnyLeft | HidNpadButton_AnyRight))
                    this->m_clickAnimationProgress = 0;

                return Element::onClick(keys);
            }

            virtual bool onTouch(TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
                if (event == TouchEvent::Touch)
                    this->m_touched = this->inBounds(currX, currY);

                if (event == TouchEvent::Release && this->m_touched) {
                    this->m_touched = false;

                    if (Element::getInputMode() == InputMode::Touch) {
                        bool handled = this->onClick(HidNpadButton_A);

                        this->m_clickAnimationProgress = 0;
                        return handled;
                    }
                }

                return false;
            }

            virtual void drawFocusBackground(gfx::Renderer *renderer) override {
                if (this->m_clickAnimationProgress > 0) {
                    this->drawClickAnimation(renderer);
                    this->m_clickAnimationProgress--;
                }
            }

            // 添加 shakeAnimation 辅助函数
            s32 shakeAnimation(u64 t_ns, s32 amplitude) {
                // 简单的震动动画
                u64 t_ms = t_ns / 1'000'000; // 转换为毫秒
                if (t_ms < 50) return amplitude;
                if (t_ms < 100) return -amplitude;
                return 0;
            }

            virtual void drawHighlight(gfx::Renderer *renderer) override {
                static float counter = 0;
                const float progress = (std::sin(counter) + 1) / 2;
                Color highlightColor = {   static_cast<u8>((0x2 - 0x8) * progress + 0x8),
                                                static_cast<u8>((0x8 - 0xF) * progress + 0xF),
                                                static_cast<u8>((0xC - 0xF) * progress + 0xF),
                                                0xF };

                counter += 0.1F;

                s32 x = 0, y = 0;

                if (this->m_highlightShaking) {
                    // 获取当前时间（纳秒）
                    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
                    // 计算时间差（纳秒）
                    u64 t_ns = now - this->m_highlightShakingStartTime;
                    
                    if (t_ns >= 100'000'000)  // 100毫秒 = 100,000,000纳秒
                        this->m_highlightShaking = false;
                    else {
                        s32 amplitude = std::rand() % 5 + 5;

                        switch (this->m_highlightShakingDirection) {
                            case FocusDirection::Up:
                                y -= shakeAnimation(t_ns, amplitude);
                                break;
                            case FocusDirection::Down:
                                y += shakeAnimation(t_ns, amplitude);
                                break;
                            case FocusDirection::Left:
                                x -= shakeAnimation(t_ns, amplitude);
                                break;
                            case FocusDirection::Right:
                                x += shakeAnimation(t_ns, amplitude);
                                break;
                            default:
                                break;
                        }

                        x = std::clamp(x, -amplitude, amplitude);
                        y = std::clamp(y, -amplitude, amplitude);
                    }
                }

                renderer->drawString("▶", false, this->getX() + x, this->getY() + y + 27, 20, a(highlightColor));
            }

            virtual void setFocused(bool state) override {
                this->m_scroll = false;
                this->m_scrollOffset = 0;
                this->m_scrollAnimationCounter = 0;
                Element::setFocused(state);
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                return this;
            }

            /**
             * @brief Sets the left hand description text of the list item
             *
             * @param text Text
             */
            inline void setText(const std::string& text) {
                this->m_text = text;
                this->m_scrollText = "";
                this->m_ellipsisText = "";
                this->m_maxWidth = 0;
            }

            /**
             * @brief Sets the right hand value text of the list item
             *
             * @param value Text
             * @param faint Should the text be drawn in a glowing green or a faint gray
             */
            inline void setValue(const std::string& value, bool faint = false) {
                this->m_value = value;
                this->m_faint = faint;
                this->m_maxWidth = 0;
            }

            /**
             * @brief Gets the left hand description text of the list item
             *
             * @return Text
             */
            inline const std::string& getText() const {
                return this->m_text;
            }

            /**
             * @brief Gets the right hand value text of the list item
             *
             * @return Value
             */
            inline const std::string& getValue() {
                return this->m_value;
            }

        protected:
            std::string m_text;
            std::string m_value = "";
            std::string m_scrollText = "";
            std::string m_ellipsisText = "";

            bool m_scroll = false;
            bool m_trunctuated = false;
            bool m_faint = false;
            bool m_touched = false;

            u16 m_maxScroll = 0;
            u16 m_scrollOffset = 0;
            u32 m_maxWidth = 0;
            u32 m_textWidth = 0;
            u16 m_scrollAnimationCounter = 0;
            u16 m_clickAnimationProgress = 0;
            
            // 修改为 u64 类型，存储纳秒时间戳
            u64 m_highlightShakingStartTime = 0;
            bool m_highlightShaking = false;
            FocusDirection m_highlightShakingDirection = FocusDirection::None;
        };

        /**
         * @brief A togglable item that goes into a list
         *
         */
        class CustomMiniToggleListItem : public CustomMiniListItem {
        public:
            /**
             * @brief Constructor
             *
             * @param text Initial description text
             */
            CustomMiniToggleListItem(const std::string& text, bool initialState, const std::string& onValue = "On", const std::string& offValue = "Off")
                : CustomMiniListItem(text), m_state(initialState), m_onValue(onValue), m_offValue(offValue) {

                this->setState(this->m_state);
            }
            virtual ~CustomMiniToggleListItem() {}

            virtual bool onClick(u64 keys) override {
                if (keys & HidNpadButton_A) {
                    this->m_state = !this->m_state;

                    this->setState(this->m_state);
                    this->m_stateChangedListener(this->m_state);

                    return CustomMiniListItem::onClick(keys);
                }

                return false;
            }

            /**
             * @brief Gets the current state of the toggle
             *
             * @return State
             */
            virtual inline bool getState() {
                return this->m_state;
            }

            /**
             * @brief Sets the current state of the toggle. Updates the Value
             *
             * @param state State
             */
            virtual void setState(bool state) {
                this->m_state = state;

                if (state)
                    this->setValue(this->m_onValue, false);
                else
                    this->setValue(this->m_offValue, true);
            }

            /**
             * @brief Adds a listener that gets called whenever the state of the toggle changes
             *
             * @param stateChangedListener Listener with the current state passed in as parameter
             */
            void setStateChangedListener(std::function<void(bool)> stateChangedListener) {
                this->m_stateChangedListener = stateChangedListener;
            }

        protected:
            bool m_state = true;
            std::string m_onValue, m_offValue;

            std::function<void(bool)> m_stateChangedListener = [](bool){};
        };
    }
}

#endif // MINILIST_HPP