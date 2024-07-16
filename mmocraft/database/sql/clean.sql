:r parameters.sql

USE master;
GO

-- Drop database
IF EXISTS (SELECT NAME from sys.databases WHERE NAME = '$(DatabaseName)')
BEGIN
ALTER DATABASE $(DatabaseName) SET SINGLE_USER WITH ROLLBACK IMMEDIATE
DROP DATABASE $(DatabaseName);
END
GO

-- Drop login
IF EXISTS (SELECT name FROM master.sys.server_principals WHERE name = '$(ServerLoginName)')
BEGIN
DROP LOGIN $(ServerLoginName);
END
GO

-- Drop user-defined functions
DROP FUNCTION IF EXISTS dbo.GetPasswordHash
GO

-- Drop user-defined procedures
DROP PROCEDURE IF EXISTS dbo.PlayerLogin
GO