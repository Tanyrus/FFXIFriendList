-- tests/support/bootstrap.lua
-- Test-environment bootstrap.
--
-- The addon runs inside Ashita, which provides a few libraries that are NOT in
-- this repo: `sugar` (provides the global T{} helper) and `json` (Ashita's JSON
-- lib). Outside the client those requires fail, which previously broke the whole
-- test suite at load time. This bootstrap registers lightweight, behaviour-
-- compatible stand-ins via package.preload so the real modules can be required
-- and exercised by `lua tests/app/run_all_tests.lua`.
--
-- Require this FIRST from any test entrypoint.

-- --- sugar: provide the global T{} table helper -----------------------------
if not package.preload["sugar"] then
    package.preload["sugar"] = function()
        if not _G.T then
            -- Minimal T{}: returns the passed table unchanged. The addon only
            -- uses it as a table constructor marker.
            _G.T = function(t) return t or {} end
        end
        return {}
    end
end
-- Ensure T exists even if sugar is never explicitly required.
if not _G.T then
    _G.T = function(t) return t or {} end
end

-- --- json: minimal, correct pure-Lua JSON encode/decode ---------------------
if not package.preload["json"] then
    package.preload["json"] = function()
        return require("tests.support.json")
    end
end

-- --- common: Ashita's bootstrap lib, required for side effects only ----------
-- libs/settings.lua does `require('common')` but uses none of its members; a
-- bare table lets settings.lua (and anything that pulls it in, e.g. the Tags
-- feature) load under the test runner.
if not package.preload["common"] then
    package.preload["common"] = function()
        return {}
    end
end

return true
