# NumTycoon

A full business/city tycoon built for the NumWorks calculator and integrated into NumPlay.

## Gameplay

Start with **$1,200**, a small reputation and an empty city. Build and optimize a portfolio of:

- Shops: cheap, reliable cash flow.
- Cafes: stronger revenue and reputation.
- Hotels: high-value tourism business.
- Arcades: entertainment and premium margins.
- Offices: expensive late-game infrastructure.

The simulation advances automatically by days. Each day calculates demand, revenue, upkeep, reputation and experience.

### Management systems

- 48 expandable city plots with roads and animated visitors.
- 5 building classes with different costs, revenue and upkeep.
- Dynamic pricing of construction based on progression.
- Reputation from 0–100 affecting demand.
- Marketing and research progression.
- 7 permanent research upgrades.
- Loans with a five-level credit limit and daily repayment.
- XP and company levels.
- Prestige after level 10, resetting the empire in exchange for a permanent prestige bonus.
- Random market events.
- Dashboard with portfolio, income and progression statistics.
- Automatic and manual saving through NumWorks persistent storage.
- Keyboard navigation using arrows / number-pad equivalents.
- Designed around the 320×240 screen with a single-frame UI and integer-heavy simulation.

## Controls

| Key | Action |
|---|---|
| Arrows | Navigate |
| OK / EXE | Select / build |
| Back | Return |
| Left / Right in city | Open Finance / Research |
| Home | Quit and save |

## Performance

The game avoids floating point simulation, uses compact save data, keeps the simulation lightweight and redraws only simple calculator-sized primitives. It is intended to remain responsive on-device while retaining a rich management loop.

## License

The code follows the GPLv3 license of the NumPlay project.
