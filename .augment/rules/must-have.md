---
type: "always_apply"
---

## Cross-Platform Compatibility Rule

All code **must be platform-independent** and **not rely on machine-specific configurations**.  

### Requirements:
- ✅ Use **standard libraries** or **well-maintained cross-platform dependencies**.  
- ✅ Avoid hardcoded file paths, environment variables, or OS-specific commands.  
- ✅ Scripts must run consistently on **Linux, macOS, and Windows** unless explicitly scoped otherwise.  
- ✅ Provide **containerization (e.g., Docker)** or reproducible environments when platform parity is critical.  
- ✅ Use **relative paths** instead of absolute ones.  
- ✅ Ensure all build scripts, tests, and tooling work in **CI/CD pipelines** independent of local machines.  
- ✅ Document any unavoidable platform-specific requirements and provide fallbacks or alternatives.  

### Enforcement:
- PRs will be reviewed for cross-platform assumptions.  
- Automated tests will run on multiple OS targets where feasible.  
- Violations must be fixed before merge.  
