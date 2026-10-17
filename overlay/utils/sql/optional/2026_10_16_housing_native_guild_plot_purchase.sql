-- Explicit reviewed emulator policy only; no original tariff or funding claim.
-- Existing personal policy/receipt bytes remain unchanged. No policy is enabled.
ALTER TABLE housing_native_plot_purchase_policies
 ADD COLUMN IF NOT EXISTS allow_guild_zero_copper TINYINT UNSIGNED NOT NULL DEFAULT 0,
 ADD COLUMN IF NOT EXISTS reviewed_minimum_guild_members INT UNSIGNED NOT NULL DEFAULT 0;
