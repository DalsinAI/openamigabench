# Design: the window look (Open light)

What the Open light look must be on every window, requester and pop-up of the desktop. It is the user's own description on 9 and 10 October 2026, and the reference picture `reference/target-look-openfiles.png` ("this is how it's supposed to look": Workbench 3.2.3, Open light, OpenFiles). OpenLook draws it (opengadtools `look/openlook.c`), the themes carry the colours (`THEME_SPEC.md`, `themes/Open.theme`), and the Team's own programs (OpenDock, OpenTitle, OpenPrefs Sound, Tata) must follow it where their own code picks colours or sizes.

This repository had no window design before this note. `DESIGN.md` here is about `workbench.library` and `icon.library`; the colours and keys are opengadtools'. Where the two disagree this note says so.

## The rule

On a themed screen **no OS grey shows anywhere**. The OS's pen 0 grey, its two-colour checkerboard and its ghost dots belong to the Classic theme only. Window bodies, lists, string gadgets and panels take the theme's colours: a light window body, white lists. There are no checkerboards and no grey boxes behind text.

## The points

| # | What it must be | What the code does today |
| --- | --- | --- |
| 1 | **Solid fills.** A requester's inside is one solid colour, the window's. No checkerboard, no rows of dots on a scaled screen | Done for the OS's own requesters (opengadtools #36: Intuition's 0xAAAA/0x5555 fill of the whole inside is painted in the window colour). Programs that draw their own dither stay as they draw it |
| 2 | **Text without grey boxes.** Text in a window never sits on a grey cell; a shorter line over a longer one leaves nothing behind | Done for windows OpenLook opened (opengadtools #37: Text in JAM2 over pen 0 gets a cell in the window colour, and RectFill in pen 0 is the window colour). Windows with a backfill of their own (OpenUp Setup, Tata, the Team's pop-ups) are not touched by it and still show grey; the leftover characters in Tata's list are that program's own drawing (not fixed) |
| 3 | **Slim window edges.** A one-pixel frame in the theme's frame colour and a light inner line; no coloured border, no bevels. The borders are the window's colour | Matches in the lab for Workbench drawer windows, as in the target. Not checked on every program's window |
| 4 | **Scroll bars only when there is something to scroll, and slim.** `scrollers auto` hides an idle scroller; the bars and arrows are the theme's, not the OS's wide bordered ones | Auto works for border scrollers and list scrollers (`prop_idle`). Scrollers drawn by a program itself (Tata's list) are the OS's. Not verified against the target on every window |
| 5 | **No size gadget** on windows (`sizegadget off`; OpenWindows resizes from the edges), no zoom gadget. Title bar: the accent colour (#2f6fb3) with white bold title text and white gadget glyphs for the active window, light grey (#cdd4db) with dark text for the others | Matches in the lab in light mode. Dark themes: titles and gadgets draw correctly in Open, Graphite and Glass in the lab. The "wrong colours when not white" reports were not reproduced there |
| 6 | **Workbench as a backdrop**: no frame, no scroll bars, no resize on the root | The OpenDock part installs `WBConfig.prefs` with the backdrop on. The target picture shows the root in a window with scroll bars, so an install without OpenDock leaves it off. Not yet a look default |
| 7 | **The screen bar keeps the standard height** for the screen font; only windows' title bars get extra height | Fixed in the defaults (openamigaup #75, amigachrome #389: `TITLEBARS 0 2` instead of `2 2`; the tool already took the two numbers apart) |
| 8 | **Disabled gadgets** are a solid fade towards the window colour, not the OS's dotted ghost | Done (opengadtools #37: `look_fade`, 55 per cent) |
| 9 | **Values fit**: "55%" and "100%" show whole in the Sound window and the speaker pop-up with CGTriumvirate 13 | Done (openamigaprefs: six characters of room, shorter sliders) |
| 10 | **Pop-ups** (the speaker's levels, the network's) take the window colour and the theme's frame, not a grey panel with a plain frame | Their panel fill is pen 0, which point 2 now covers where OpenLook opened the window. The frame is still the OS's DrawBevelBox |
| 11 | **The dock** is a translucent rounded shelf tight to its icons, with no grey slab or border around it, and about 4 pixels less room above and below the icons | Not done. The dock shows a grey rectangle where it could not copy what is behind it, and its padding is larger than the target's |
| 12 | **The screen bar's icons** (the logo, Tata's shield and the other taskspace icons) are centred in the bar, drawn with their transparency over the bar's colour (no white box), and sit under windows that overlap the bar | Not done. They are drawn straight into the screen, so they show over windows, with a white box, and too high |
| 13 | **Icons on a coloured or dark dock** are blended over the dock, not against white | Not done (OpenDock draws icons over black and white to find their transparency; the white rectangle in point 12 is the same family) |
| 14 | **Programs never keep a window open when Workbench closes** ("Intuition is attempting to reset the Workbench screen. Please close the following windows: <No title>") | Not done: one of the Team's untitled windows stays open; not yet found which |

## The target picture

`reference/target-look-openfiles.png`: a flat white window inside, a solid blue title bar with white bold title text and white gadgets, a thin clean border, a light toolbar row, a clean sans screen font, dark sans text on the screen bar and the translucent rounded dock tight to its icons. It also shows the root as a window with scroll bars; the user's word on that is that it must not: Workbench is a backdrop.

## Pens and the palette

Changing pen 0 in the screen's palette to the window colour (one change that would cover every grey) does not work on OpenRTG's 16-bit screen: `SetRGB32` on the Workbench screen's pen 0 left every grey as it was. The look is therefore drawn by OpenLook's hooks, window by window.
