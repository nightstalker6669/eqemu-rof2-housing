-- Optional additive escrow foundation; requires001 and004. No content/rule seeds.
-- Only explicitly tagged isolated synthetic harness actors may mutate these tables.
-- Integer copper is authoritative; doubles belong only to native display snapshots.
CREATE TABLE IF NOT EXISTS housing_escrow_balances (
    estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
    copper BIGINT NOT NULL DEFAULT 0,
    CONSTRAINT housing_escrow_nonnegative CHECK (copper>=0),
    CONSTRAINT housing_escrow_estate_fk FOREIGN KEY (estate_id)
        REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_escrow_transfers (
    request_key VARBINARY(96) NOT NULL PRIMARY KEY,
    character_id INT UNSIGNED NOT NULL,
    account_id INT UNSIGNED NOT NULL,
    estate_id INT UNSIGNED NOT NULL,
    action TINYINT UNSIGNED NOT NULL,
    denomination TINYINT UNSIGNED NOT NULL,
    quantity BIGINT NOT NULL,
    before_copper INT NOT NULL,
    before_silver INT NOT NULL,
    before_gold INT NOT NULL,
    before_platinum INT NOT NULL,
    after_copper INT NOT NULL DEFAULT 0,
    after_silver INT NOT NULL DEFAULT 0,
    after_gold INT NOT NULL DEFAULT 0,
    after_platinum INT NOT NULL DEFAULT 0,
    copper_transferred BIGINT NOT NULL DEFAULT 0,
    escrow_before BIGINT NOT NULL DEFAULT 0,
    escrow_after BIGINT NOT NULL DEFAULT 0,
    completed TINYINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX housing_escrow_actor (character_id),
    INDEX housing_escrow_estate (estate_id),
    CONSTRAINT housing_escrow_transfer_bounds CHECK
        (action<=1 AND denomination<=3 AND quantity>0 AND quantity<=2147483647 AND
         before_copper>=0 AND before_silver>=0 AND before_gold>=0 AND before_platinum>=0 AND
         after_copper>=0 AND after_silver>=0 AND after_gold>=0 AND after_platinum>=0 AND
         copper_transferred>=0 AND escrow_before>=0 AND escrow_after>=0 AND completed<=1)
) ENGINE=InnoDB;
-- Receipts have no cascading estate FK: committed immutable audit evidence must
-- not disappear on deletion. Estate/balance custody and retention need a later policy.
-- Pending records are NEVER committed separately from wallet/balance/receipt.
