DROP TABLE IF EXISTS db_settings;

CREATE TABLE IF NOT EXISTS db_settings (
    version       INTEGER NOT NULL,
    name          TEXT NOT NULL CHECK( LENGTH(name) <= 100 ),
    paranoic      INTEGER NOT NULL,
    current_media INTEGER
);

INSERT INTO db_settings (version, name, paranoic) VALUES (9, 'my archive', 1);

DROP TABLE IF EXISTS arc_tree;
CREATE TABLE IF NOT EXISTS arc_tree(
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER,
    typ       TEXT NOT NULL CHECK( typ in ('folder', 'file') ),
    name      TEXT NOT NULL,
    siz       INTEGER,
    hash      TEXT,
    lnk       TEXT,
    dt        INTEGER NOT NULL, /* date when put to database */
    dt_org    INTEGER,          /* original m_mtime */
    perm      INTEGER,          /* original permissions */
    UNIQUE (name, parent_id)
);

CREATE INDEX arc_tree_walking ON arc_tree (typ, parent_id);
INSERT INTO arc_tree (id, parent_id, typ, name, dt) VALUES (1, 0, 'folder', '/', 0);

DROP TABLE IF EXISTS arc_media;
CREATE TABLE IF NOT EXISTS arc_media (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    capacity  INTEGER NOT NULL,
    occupied  INTEGER NOT NULL,
    locked    INTEGER NOT NULL,
    name      TEXT NOT NULL,
    serial    TEXT NOT NULL UNIQUE,
    rockridge INTEGER NOT NULL,
    joliet    INTEGER NOT NULL
);

DROP TABLE IF EXISTS arc_tree_to_media;
CREATE TABLE IF NOT EXISTS arc_tree_to_media (
    arc_tree_id  INTEGER NOT NULL,
    arc_media_id INTEGER NOT NULL,
    UNIQUE (arc_tree_id, arc_media_id)
);

DROP TABLE IF EXISTS arc_history;
CREATE TABLE IF NOT EXISTS arc_history (id INTEGER PRIMARY KEY AUTOINCREMENT, description TEXT, dt INTEGER NOT NULL, data BLOB NOT NULL, dsize INTEGER NOT NULL, hash TEXT NOT NULL);
