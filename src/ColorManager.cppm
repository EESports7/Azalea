module;

#include "include.hpp"

export module ColorManager;

struct ColorKey {
    static constexpr auto Holding       = "holding-color"sv;
    static constexpr auto Release       = "release-color"sv;
    static constexpr auto Mixed         = "mixed-color"sv;
    static constexpr auto InnerHitbox   = "inner-hitbox-color"sv;
    static constexpr auto OuterHitbox   = "outer-hitbox-color"sv;
    static constexpr auto RotatedHitbox = "rotated-hitbox-color"sv;
    static constexpr auto CircleHitbox  = "circle-hitbox-color"sv;
};

export enum class ColorIdx {
    Holding,
    Release,
    Mixed,
    InnerHitbox,
    OuterHitbox,
    RotatedHitbox,
    CircleHitbox
};

export class ColorManager {
    ColorManager() = default;

    ccColor3B Holding;
    ccColor3B Release;
    ccColor3B Mixed;
    ccColor3B InnerHitbox;
    ccColor3B OuterHitbox;
    ccColor3B RotatedHitbox;
    ccColor3B CircleHitbox;

public:
    ColorManager(const ColorManager&) = delete;
    ColorManager& operator=(const ColorManager&) = delete;
    ColorManager(ColorManager&&) = delete;
    ColorManager& operator=(ColorManager&&) = delete;

    static auto& get(){
        static ColorManager i;
        return i;
    }

    void loadColors(){
        auto mod = Mod::get();

        Holding       = mod->getSettingValue<ccColor3B>(ColorKey::Holding);
        Release       = mod->getSettingValue<ccColor3B>(ColorKey::Release);
        Mixed         = mod->getSettingValue<ccColor3B>(ColorKey::Mixed);
        InnerHitbox   = mod->getSettingValue<ccColor3B>(ColorKey::InnerHitbox);
        OuterHitbox   = mod->getSettingValue<ccColor3B>(ColorKey::OuterHitbox);
        RotatedHitbox = mod->getSettingValue<ccColor3B>(ColorKey::RotatedHitbox);
        CircleHitbox  = mod->getSettingValue<ccColor3B>(ColorKey::CircleHitbox);
    }

    ccColor4F getColor(ColorIdx idx){
        switch(idx){
            case ColorIdx::Holding:       return ccc4FFromccc3B(Holding);
            case ColorIdx::Release:       return ccc4FFromccc3B(Release);
            case ColorIdx::Mixed:         return ccc4FFromccc3B(Mixed);
            case ColorIdx::InnerHitbox:   return ccc4FFromccc3B(InnerHitbox);
            case ColorIdx::OuterHitbox:   return ccc4FFromccc3B(OuterHitbox);
            case ColorIdx::RotatedHitbox: return ccc4FFromccc3B(RotatedHitbox);
            case ColorIdx::CircleHitbox:  return ccc4FFromccc3B(CircleHitbox);
            default:                      return ccc4f(0, 0, 0, 0);
        }
    }
};