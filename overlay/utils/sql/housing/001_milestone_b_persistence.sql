-- Optional, additive Milestone B schema. No automatic installation, rule changes,
-- fabricated plot seeds, existing instance rewrites, or destructive initialization.
-- PersistenceService is disabled unless its world owner explicitly enables it.
CREATE TABLE IF NOT EXISTS housing_instance_allocator (
    id TINYINT UNSIGNED NOT NULL PRIMARY KEY,
    last_instance_id INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB;
INSERT IGNORE INTO housing_instance_allocator (id) VALUES (1);

CREATE TABLE IF NOT EXISTS housing_neighborhoods (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    request_key VARBINARY(96) NOT NULL,
    name VARCHAR(128) NOT NULL,
    zone_id INT UNSIGNED NOT NULL,
    zone_version TINYINT UNSIGNED NOT NULL,
    instance_id SMALLINT UNSIGNED NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY housing_neighborhood_request (request_key),
    UNIQUE KEY housing_neighborhood_instance (instance_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS housing_plot_definitions (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    neighborhood_id INT UNSIGNED NOT NULL,
    plot_number INT UNSIGNED NOT NULL,
    label VARCHAR(128) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY housing_plot_number (neighborhood_id, plot_number),
    CONSTRAINT housing_plot_neighborhood FOREIGN KEY (neighborhood_id)
        REFERENCES housing_neighborhoods (id) ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS housing_estates (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    request_key VARBINARY(96) NOT NULL,
    plot_id INT UNSIGNED NOT NULL,
    owner_kind TINYINT UNSIGNED NOT NULL,
    owner_id INT UNSIGNED NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY housing_estate_request (request_key),
    active TINYINT UNSIGNED NOT NULL DEFAULT 1,
    active_plot_id INT UNSIGNED AS (CASE WHEN active=1 THEN plot_id ELSE NULL END) STORED,
    UNIQUE KEY housing_estate_active_plot (active_plot_id),
    KEY housing_estate_plot_history (plot_id),
    CONSTRAINT housing_estate_active_flag CHECK (active<=1),
    CONSTRAINT housing_estate_plot_fk FOREIGN KEY (plot_id)
        REFERENCES housing_plot_definitions (id) ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Instance metadata is validated through the service instead of a foreign key
-- to legacy instance_list, whose id type/engine varies across supported schemas.
-- No interiors are created by plot ownership. Future house custody/placement
-- migrations must give each house its own persistent interior identity.

-- Current authority is distinct from immutable historical ownership generations.
CREATE OR REPLACE ALGORITHM=MERGE VIEW housing_current_estates AS
 SELECT id,request_key,plot_id,owner_kind,owner_id,created_at FROM housing_estates WHERE active=1;
