-- Additive opt-in. No current policy or property is changed by this migration.
-- Requires native personal/guild plot purchase and upkeep enrollment schemas.
-- Paid guild plots use the current authorized guild leader's carried coin only;
-- retain exact catalogue/hash review, action27, member minimum and profile journal.
ALTER TABLE housing_native_plot_purchase_policies
 ADD COLUMN IF NOT EXISTS allow_guild_carried_copper TINYINT UNSIGNED NOT NULL DEFAULT 0;
