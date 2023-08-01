// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_GUI_STATIC_TEXT_H_INCLUDED__
#define __C_GUI_STATIC_TEXT_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUIStaticText.h"
#include "irrArray.h"
#include "GlyphLayout.h"

namespace irr
{
namespace gui
{
	class CGUIStaticText : public IGUIStaticText
	{
	public:

		//! constructor
		CGUIStaticText(const core::stringw& text, bool border, IGUIEnvironment* environment,
			IGUIElement* parent, s32 id, const core::rect<s32>& rectangle,
			bool background = false);

		//! destructor
		virtual ~CGUIStaticText();

		//! draws the element and its children
		virtual void draw();

		//! Sets another skin independent font.
		virtual void setOverrideFont(IGUIFont* font=0);

		//! Gets the override font (if any)
		virtual IGUIFont* getOverrideFont() const;

		//! Get the font which is used right now for drawing
		virtual IGUIFont* getActiveFont() const;

		//! Sets another color for the text.
		virtual void setOverrideColor(video::SColor color);

		//! Sets another color for the background.
		virtual void setBackgroundColor(video::SColor color);

		//! Sets whether to draw the background
		virtual void setDrawBackground(bool draw);

		//! Gets the background color
		virtual video::SColor getBackgroundColor() const;

		//! Checks if background drawing is enabled
		virtual bool isDrawBackgroundEnabled() const;

		//! Sets whether to draw the border
		virtual void setDrawBorder(bool draw);

		//! Checks if border drawing is enabled
		virtual bool isDrawBorderEnabled() const;

		//! Sets alignment mode for text
		virtual void setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical);

		//! Gets the override color
		virtual video::SColor getOverrideColor() const;

		//! Sets if the static text should use the overide color or the
		//! color in the gui skin.
		virtual void enableOverrideColor(bool enable);

		//! Checks if an override color is enabled
		virtual bool isOverrideColorEnabled() const;

		//! Set whether the text in this label should be clipped if it goes outside bounds
		virtual void setTextRestrainedInside(bool restrainedInside);

		//! Checks if the text in this label should be clipped if it goes outside bounds
		virtual bool isTextRestrainedInside() const;

		//! Enables or disables word wrap for using the static text as
		//! multiline text control.
		virtual void setWordWrap(bool enable);

		//! Checks if word wrap is enabled
		virtual bool isWordWrapEnabled() const;

		//! Sets the new caption of this element.
		virtual void setText(const core::stringw& text);

		//! Returns the height of the text in pixels when it is drawn.
		virtual s32 getTextHeight() const;

		//! Returns the width of the current text, in the current font
		virtual s32 getTextWidth() const;

		//! Updates the absolute position, splits text if word wrap is enabled
		virtual void updateAbsolutePosition();

		//! Writes attributes of the element.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

		//! Reads attributes of the element
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

		virtual const std::vector<GlyphLayout>& getGlyphLayouts() const { return m_glyph_layouts; }
		virtual void setGlyphLayouts(std::vector<GlyphLayout>& gls) { m_glyph_layouts = gls; }
		virtual void clearGlyphLayouts() { m_glyph_layouts.clear(); }
		virtual void setUseGlyphLayoutsOnly(bool gls_only) { m_use_glyph_layouts_only = gls_only; }
		virtual bool useGlyphLayoutsOnly() const { return m_use_glyph_layouts_only; }
		//! called if an event happened.
		virtual bool OnEvent(const SEvent& event);
		virtual void setMouseCallback(std::function<bool(IGUIStaticText* text, SEvent::SMouseInput)> cb) { m_callback = cb; }
		virtual s32 getCluster(int x, int y, std::shared_ptr<std::u32string>* out_orig_str, int* out_glyph_idx = NULL);
	private:

		//! Breaks the single text line.
		void breakText();
		void getDrawPosition(core::rect<s32>* draw_pos, bool* hcenter, const core::rect<s32>** clip);

		EGUI_ALIGNMENT HAlign, VAlign;
		bool Border;
		bool OverrideColorEnabled;
		bool OverrideBGColorEnabled;
		bool WordWrap;
		bool Background;
		bool RestrainTextInside;

		video::SColor OverrideColor, BGColor;
		gui::IGUIFont* OverrideFont;
		gui::IGUIFont* LastBreakFont; // stored because: if skin changes, line break must be recalculated.

		//! If true, setText / updating this element will not reshape with Text object
		bool m_use_glyph_layouts_only;
		std::vector<GlyphLayout> m_glyph_layouts;
		std::function<bool(IGUIStaticText* text, irr::SEvent::SMouseInput)> m_callback;
	};

} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_

#endif

