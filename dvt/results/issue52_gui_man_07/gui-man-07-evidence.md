# GUI-MAN-07 Evidence

- Issue: `#52`
- Executed: `2026-05-06T12:24:22+01:00`
- Executable: `build\patient_monitor_gui.exe`
- Result: `PASS`

## Checklist

| Check | Result | Evidence |
| --- | --- | --- |
| Active patient context is visible in the header | PASS | `build\gui_review_issue52\01-active-default.png` showed the header card populated with `James Mitchell`, `ID 2001`, and `Age 45` while the patient bar and edit fields showed the same patient context. |
| Title/card split responds to available width | PASS | The default-width capture (`01-active-default.png`) showed a materially wider title region than the minimum-width capture (`02-active-min-width.png`), while the card, role badge, and buttons remained non-overlapping in both states. |
| Timer-driven updates keep the same patient identity | PASS | `03-active-after-timer.png` showed the reading count advance from `1 / 10` to `3 / 10` and the trend/history data update while the header card remained `James Mitchell / ID 2001 / Age 45`. |
| Clear Session immediately clears the card | PASS | `04-clear-immediate.png` showed the header card switch to `No active patient`, the patient bar switch to `Awaiting first simulation reading...`, and the tiles/history surfaces clear to empty-session placeholders. |
| Clear Session stays empty across the next timer tick | PASS | `05-clear-after-timer.png` captured the dashboard again after 2.5 seconds with simulation still live; the card remained `No active patient` and no zeroed or phantom patient record reappeared. |
| Device mode clears patient context and uses the reclaimed header space | PASS | `06-device-mode.png` showed `No active patient`, `N/A` tiles, the hidden Pause button gap removed, and the full `Patient Vital Signs Monitor` title visible in the header. |
| Logout removes patient identity from the UI | PASS | `07-logout-login.png` showed the login window after logout with no dashboard/patient content still visible. |

## Notes

- The default simulation-on header still truncates the app title slightly because the header is carrying the title, identity card, user/status text, role badge, and three command buttons at the same time.
- The reviewed fix removed the previous hard-coded `108px` title slot. The default and minimum-width captures demonstrate that the title region now expands and contracts with the real available width, and device mode confirms that hidden-button space is reclaimed instead of being left blank.
