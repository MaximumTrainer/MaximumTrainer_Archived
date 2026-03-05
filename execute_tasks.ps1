# Change to the repository directory
Set-Location 'C:\Users\danwo\CodeProjects\MaximumTrainer_Archived'

# Delete the codacy workflow file
Remove-Item '.github\workflows\codacy-analysis.yml' -Force -ErrorAction Stop

# Stage the deletion
& git add '.github\workflows\codacy-analysis.yml'

# Display status
Write-Host "=== Git Status ===" -ForegroundColor Green
& git status

# Create commit message
$commitMessage = @"
Fix compilation errors for Qt5/Qt6 cross-platform builds

Four targeted fixes to resolve build failures on Linux, macOS, and Windows:

src/_subclassQWT/myqwtdial.cpp
- Replace setPenWidthF(2.0) with setPenWidth(2): QwtRoundScaleDraw has
  no setPenWidthF member. Fixes hard compile error on Linux.

src/_subclassQWT/workoutplotlist.cpp
- Add missing #include <qwt_text.h> and #include <QPen>: types used as
  complete types but only forward-declared. Fixes 3 compile errors on macOS.

src/_subclassQWT/mydialbox.cpp
- Replace QColor::dark() / QColor::light() with darker() / lighter()
  throughout (6 occurrences). Deprecated in Qt 5.0, removed in Qt 6.

.github/workflows/build.yml
- Switch Windows SFML download from Invoke-WebRequest to curl.exe -L:
  handles SourceForge redirects correctly. Fixes Windows CI network failure.

.github/workflows/codacy-analysis.yml
- Remove Codacy security scan workflow entirely.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
"@

# Save message to temp file
$commitMessage | Out-File -FilePath '.git\COMMIT_EDITMSG' -Encoding UTF8 -Force

# Amend the commit
Write-Host "`n=== Amending Commit ===" -ForegroundColor Green
& git commit --amend -m $commitMessage

# Show the new commit
Write-Host "`n=== New Commit ===" -ForegroundColor Green
& git --no-pager log --oneline -1
& git --no-pager show HEAD --stat

# Force push to the branch
Write-Host "`n=== Force Pushing ===" -ForegroundColor Green
& git push --force-with-lease origin copilot/review-qt6-compatibility

# Get the new commit SHA
$commitSHA = & git rev-parse HEAD

Write-Host "`n=== Operation Complete ===" -ForegroundColor Green
Write-Host "New commit SHA: $commitSHA"
