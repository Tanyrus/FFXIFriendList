# Test script to verify the array slicing fix in Install-FFXIFriendList.bat

function Test-ArraySlicing {
    param(
        [string]$TestName,
        [string[]]$OriginalLines,
        [int]$InsertIndex,
        [string]$NewCommand
    )
    
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "Test: $TestName" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    
    Write-Host "`nOriginal lines ($($OriginalLines.Count) total):" -ForegroundColor Yellow
    for ($i = 0; $i -lt $OriginalLines.Count; $i++) {
        Write-Host "  [$i] $($OriginalLines[$i])"
    }
    
    Write-Host "`nInserting at index: $InsertIndex" -ForegroundColor Yellow
    Write-Host "New command: $NewCommand" -ForegroundColor Yellow
    
    # OLD (BUGGY) LOGIC
    Write-Host "`n--- OLD LOGIC (Buggy) ---" -ForegroundColor Red
    try {
        $oldLines = @($OriginalLines[0..($InsertIndex - 1)]) + $NewCommand + @($OriginalLines[$InsertIndex..($OriginalLines.Count - 1)])
        Write-Host "Result ($($oldLines.Count) lines):" -ForegroundColor Green
        for ($i = 0; $i -lt $oldLines.Count; $i++) {
            Write-Host "  [$i] $($oldLines[$i])"
        }
        if ($oldLines.Count -ne $OriginalLines.Count + 1) {
            Write-Host "ERROR: Line count mismatch! Expected $($OriginalLines.Count + 1), got $($oldLines.Count)" -ForegroundColor Red
        }
    } catch {
        Write-Host "ERROR: $_" -ForegroundColor Red
    }
    
    # NEW (FIXED) LOGIC
    Write-Host "`n--- NEW LOGIC (Fixed) ---" -ForegroundColor Green
    try {
        if ($InsertIndex -eq 0) {
            $newLines = @($NewCommand) + @($OriginalLines)
        } elseif ($InsertIndex -lt $OriginalLines.Count) {
            $newLines = @($OriginalLines[0..($InsertIndex - 1)]) + $NewCommand + @($OriginalLines[$InsertIndex..($OriginalLines.Count - 1)])
        } else {
            $newLines = @($OriginalLines[0..($InsertIndex - 1)]) + $NewCommand
        }
        Write-Host "Result ($($newLines.Count) lines):" -ForegroundColor Green
        for ($i = 0; $i -lt $newLines.Count; $i++) {
            Write-Host "  [$i] $($newLines[$i])"
        }
        if ($newLines.Count -ne $OriginalLines.Count + 1) {
            Write-Host "ERROR: Line count mismatch! Expected $($OriginalLines.Count + 1), got $($newLines.Count)" -ForegroundColor Red
        } else {
            Write-Host "SUCCESS: All lines preserved!" -ForegroundColor Green
        }
    } catch {
        Write-Host "ERROR: $_" -ForegroundColor Red
    }
}

# Test Case 1: Insert in middle of file
Test-ArraySlicing -TestName "Insert after middle /addon load" `
    -OriginalLines @(
        "/addon load fps",
        "/addon load autoexec",
        "/addon load distance",
        "# some comment",
        "/echo Done loading"
    ) `
    -InsertIndex 3 `
    -NewCommand "/addon load ffxifriendlist"

# Test Case 2: Insert after last line (edge case that was buggy)
Test-ArraySlicing -TestName "Insert after last /addon load (EDGE CASE)" `
    -OriginalLines @(
        "/addon load fps",
        "/addon load autoexec",
        "/addon load distance"
    ) `
    -InsertIndex 3 `
    -NewCommand "/addon load ffxifriendlist"

# Test Case 3: Insert after first line
Test-ArraySlicing -TestName "Insert after first /addon load" `
    -OriginalLines @(
        "/addon load fps",
        "# comment",
        "/echo something",
        "/addon load distance"
    ) `
    -InsertIndex 1 `
    -NewCommand "/addon load ffxifriendlist"

# Test Case 4: Insert at beginning (index 0)
Test-ArraySlicing -TestName "Insert at beginning (special case)" `
    -OriginalLines @(
        "# comment",
        "/addon load fps",
        "/echo something"
    ) `
    -InsertIndex 0 `
    -NewCommand "/addon load ffxifriendlist"

# Test Case 5: Single line file
Test-ArraySlicing -TestName "Single line file" `
    -OriginalLines @(
        "/addon load fps"
    ) `
    -InsertIndex 1 `
    -NewCommand "/addon load ffxifriendlist"

# Test Case 6: Horizon XI style - Insert after custom section header (middle of file)
Test-ArraySlicing -TestName "Horizon XI - Insert after custom section header (middle)" `
    -OriginalLines @(
        "/plugin load blinkmenot",
        "/plugin load fps",
        "# Custom user addons",
        "# Add your custom addons below",
        "/addon load distance",
        "/echo Done loading"
    ) `
    -InsertIndex 4 `
    -NewCommand "/addon load ffxifriendlist"

# Test Case 7: Horizon XI style - Insert when custom section is at end (edge case)
Test-ArraySlicing -TestName "Horizon XI - Custom section at end (EDGE CASE)" `
    -OriginalLines @(
        "/plugin load blinkmenot",
        "/plugin load fps",
        "# Custom user addons and plugins",
        "# Add your custom addons below"
    ) `
    -InsertIndex 4 `
    -NewCommand "/addon load ffxifriendlist"

# Test Case 8: Horizon XI style - Insert right after custom header (no existing addons)
Test-ArraySlicing -TestName "Horizon XI - Right after header, no addons yet" `
    -OriginalLines @(
        "/plugin load blinkmenot",
        "# Custom user addons",
        "",
        "/echo All done"
    ) `
    -InsertIndex 2 `
    -NewCommand "/addon load ffxifriendlist"

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Testing Complete" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
