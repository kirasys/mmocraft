CREATE LOGIN $(ServerLoginName) WITH 
    PASSWORD = '$(ServerLoginPassword)',
    DEFAULT_DATABASE = $(DatabaseName),
    CHECK_POLICY = OFF;
GO

USE $(DatabaseName)
CREATE USER $(DatabaseUserName) FOR LOGIN $(ServerLoginName);
GO