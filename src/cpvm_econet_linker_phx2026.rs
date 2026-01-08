use std::error::Error;
use std::fmt;
use std::fs::File;
use std::io::{BufRead, BufReader};

/// Identifier for a physical-virtual water asset node.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct NodeId(pub String);

/// Asset type taxonomy for CPVM–EcoNet nodes.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum AssetType {
    Reservoir,
    Plant,
    RiverReach,
    Basin,
    WatershedCluster,
    Other(String),
}

impl AssetType {
    pub fn from_str(s: &str) -> Self {
        match s.trim() {
            "Reservoir" => AssetType::Reservoir,
            "Plant" => AssetType::Plant,
            "RiverReach" => AssetType::RiverReach,
            "Basin" => AssetType::Basin,
            "WatershedCluster" => AssetType::WatershedCluster,
            other => AssetType::Other(other.to_string()),
        }
    }
}

/// Simple unit enum for concentration.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum ConcentrationUnit {
    NgPerL,
    MgPerL,
    MpnPer100mL,
    Other(String),
}

impl ConcentrationUnit {
    pub fn from_str(s: &str) -> Self {
        match s.trim() {
            "ng/L" => ConcentrationUnit::NgPerL,
            "mg/L" => ConcentrationUnit::MgPerL,
            "MPN/100mL" => ConcentrationUnit::MpnPer100mL,
            other => ConcentrationUnit::Other(other.to_string()),
        }
    }
}

/// Simple unit enum for flow.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum FlowUnit {
    M3PerS,
    Other(String),
}

impl FlowUnit {
    pub fn from_str(s: &str) -> Self {
        match s.trim() {
            "m3/s" => FlowUnit::M3PerS,
            other => FlowUnit::Other(other.to_string()),
        }
    }
}

/// Core CPVM node metadata and baseline environmental state.
#[derive(Debug, Clone)]
pub struct CpvmNodeMeta {
    pub node_id: NodeId,
    pub asset_type: AssetType,
    pub waterbody: String,
    pub region: String,
    /// Name of the CPVM profile (e.g. PFAS_PFBS_LP_v1).
    pub cpvm_profile: String,
    /// Baseline inlet concentration C_in.
    pub cin_baseline: f64,
    pub cin_unit: ConcentrationUnit,
    /// Average discharge Q.
    pub q_avg: f64,
    pub q_unit: FlowUnit,
    /// Time horizon for eco-impact integration [s].
    pub horizon_s: f64,
    /// CEIM-style ecoimpactscore in [0,1].
    pub ecoimpactscore: f64,
    /// Karma per unit canonical impact.
    pub karma_per_unit: f64,
    pub notes: String,
}

/// CPVM safety configuration (per-node).
#[derive(Debug, Clone)]
pub struct CpvmSafetyConfig {
    /// Safe concentration threshold C_safe.
    pub safe_threshold: f64,
    /// Reference concentration C_ref used for normalization.
    pub cref: f64,
    /// Weight on Lyapunov-type viability violations.
    pub lambda_clf: f64,
    /// Weight on barrier-type safety violations.
    pub mu_cbf: f64,
}

/// Aggregated CPVM node configuration ready for control / evaluation.
#[derive(Debug, Clone)]
pub struct CpvmNodeConfig {
    pub meta: CpvmNodeMeta,
    pub safety: CpvmSafetyConfig,
}

/// Eco-impact evaluation result for a node over its configured horizon.
#[derive(Debug, Clone)]
pub struct EcoImpactResult {
    /// Mass load avoided M = (C_in - C_out) * Q * t.
    pub mass_avoided: f64,
    /// Normalized eco-impact score in [0,1] (re-using CEIM score).
    pub ecoimpactscore: f64,
    /// Karma gain = ecoimpactscore * mass_avoided * karma_per_unit.
    pub karma_gain: f64,
}

/// Errors for CSV parsing and configuration.
#[derive(Debug)]
pub enum CpvmLinkerError {
    Io(std::io::Error),
    Parse(String),
}

impl fmt::Display for CpvmLinkerError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            CpvmLinkerError::Io(e) => write!(f, "IO error: {}", e),
            CpvmLinkerError::Parse(e) => write!(f, "Parse error: {}", e),
        }
    }
}

impl Error for CpvmLinkerError {}

impl From<std::io::Error> for CpvmLinkerError {
    fn from(err: std::io::Error) -> Self {
        CpvmLinkerError::Io(err)
    }
}

/// Parse a single CSV line into fields, honoring quoted segments.
/// This keeps dependencies minimal for embedded environments.
fn split_csv_line(line: &str) -> Vec<String> {
    let mut fields = Vec::new();
    let mut current = String::new();
    let mut in_quotes = false;

    for c in line.chars() {
        match c {
            '"' => {
                in_quotes = !in_quotes;
            }
            ',' if !in_quotes => {
                fields.push(current.trim().to_string());
                current.clear();
            }
            _ => current.push(c),
        }
    }
    if !current.is_empty() || line.ends_with(',') {
        fields.push(current.trim().to_string());
    }
    fields
}

/// Load CPVM–EcoNet qpudatashard CSV into structured node metadata.
pub fn load_cpvm_nodes_from_csv(path: &str) -> Result<Vec<CpvmNodeMeta>, CpvmLinkerError> {
    let file = File::open(path)?;
    let reader = BufReader::new(file);

    let mut lines = reader.lines();

    // Skip header
    let _header = match lines.next() {
        Some(Ok(h)) => h,
        Some(Err(e)) => return Err(CpvmLinkerError::Io(e)),
        None => return Ok(Vec::new()),
    };

    let mut nodes = Vec::new();

    for (idx, line_res) in lines.enumerate() {
        let line = line_res?;
        if line.trim().is_empty() {
            continue;
        }
        let fields = split_csv_line(&line);
        if fields.len() < 12 {
            return Err(CpvmLinkerError::Parse(format!(
                "Line {} has insufficient fields: {}",
                idx + 2,
                fields.len()
            )));
        }

        let node_id = NodeId(fields[0].to_string());
        let asset_type = AssetType::from_str(&fields[1]);
        let waterbody = fields[2].to_string();
        let region = fields[3].to_string();
        let cpvm_profile = fields[4].to_string();

        let cin_baseline: f64 = fields[5]
            .parse()
            .map_err(|e| CpvmLinkerError::Parse(format!("cin_baseline parse error: {}", e)))?;
        let cin_unit = ConcentrationUnit::from_str(&fields[6]);

        let q_avg: f64 = fields[7]
            .parse()
            .map_err(|e| CpvmLinkerError::Parse(format!("q_avg parse error: {}", e)))?;
        let q_unit = FlowUnit::from_str(&fields[8]);

        let horizon_s: f64 = fields[9]
            .parse()
            .map_err(|e| CpvmLinkerError::Parse(format!("horizon_s parse error: {}", e)))?;

        let ecoimpactscore: f64 = fields[10]
            .parse()
            .map_err(|e| CpvmLinkerError::Parse(format!("ecoimpactscore parse error: {}", e)))?;

        let karma_per_unit: f64 = fields[11]
            .parse()
            .map_err(|e| CpvmLinkerError::Parse(format!("karma_per_unit parse error: {}", e)))?;

        let notes = if fields.len() > 12 {
            fields[12..].join(",")
        } else {
            String::new()
        };

        nodes.push(CpvmNodeMeta {
            node_id,
            asset_type,
            waterbody,
            region,
            cpvm_profile,
            cin_baseline,
            cin_unit,
            q_avg,
            q_unit,
            horizon_s,
            ecoimpactscore,
            karma_per_unit,
            notes,
        });
    }

    Ok(nodes)
}

/// Construct a node-specific CPVM safety config from domain rules.
///
/// This function is intentionally simple and deterministic so that higher-level
/// governance logic can override or wrap it as needed.
pub fn derive_safety_config(
    meta: &CpvmNodeMeta,
    cref_default: f64,
    lambda_clf: f64,
    mu_cbf: f64,
) -> CpvmSafetyConfig {
    // The default safe_threshold is the baseline if nothing else is provided.
    // In a full stack, this would be min(EPA, EU, WHO, etc.) and possibly lower than baseline.
    let safe_threshold = meta.cin_baseline.min(cref_default);

    CpvmSafetyConfig {
        safe_threshold,
        cref: cref_default,
        lambda_clf,
        mu_cbf,
    }
}

/// Create a fully bound CPVM node configuration from metadata and global defaults.
///
/// In the full Cybercore-Brain stack, per-profile C_ref and weights would be
/// injected from CEIM‑XJ governance logic; this function provides a sane core.
pub fn bind_cpvm_config(
    meta: CpvmNodeMeta,
    cref_default: f64,
    lambda_clf: f64,
    mu_cbf: f64,
) -> CpvmNodeConfig {
    let safety = derive_safety_config(&meta, cref_default, lambda_clf, mu_cbf);
    CpvmNodeConfig { meta, safety }
}

/// Compute mass load avoided M = (C_in - C_out) * Q * t, using consistent units.
///
/// This assumes:
/// - C_in and C_out share the same concentration units.
/// - Q in m3/s, t in s.
/// - For Karma accounting, *relative* magnitude matters; absolute unit conversion
///   is handled at governance level if needed.
pub fn compute_mass_avoided(
    cin: f64,
    cout: f64,
    q_m3_per_s: f64,
    horizon_s: f64,
) -> f64 {
    let delta_c = (cin - cout).max(0.0);
    delta_c * q_m3_per_s * horizon_s
}

/// Evaluate eco-impact and Karma for a CPVM-controlled node over its horizon.
///
/// This function is the core bridge: controllers can propose C_out, and this
/// returns the resulting mass avoided and Karma gain consistent with CEIM-style
/// ecoimpactscore and Karma-per-unit configuration.
pub fn evaluate_ecoimpact_for_node(
    cfg: &CpvmNodeConfig,
    cout: f64,
) -> EcoImpactResult {
    let meta = &cfg.meta;
    let q_m3_per_s = match meta.q_unit {
        FlowUnit::M3PerS => meta.q_avg,
        FlowUnit::Other(_) => meta.q_avg, // assume upstream has normalized if using nonstandard units
    };

    let mass_avoided = compute_mass_avoided(
        meta.cin_baseline,
        cout,
        q_m3_per_s,
        meta.horizon_s,
    );

    let ecoimpactscore = meta.ecoimpactscore.clamp(0.0, 1.0);
    let karma_gain = ecoimpactscore * mass_avoided * meta.karma_per_unit;

    EcoImpactResult {
        mass_avoided,
        ecoimpactscore,
        karma_gain,
    }
}

/// Example helper: build configs for all nodes from a qpudatashard path.
///
/// Callers can then wire these configs into local controllers, smart-city
/// orchestrators, or basin-scale optimizers.
pub fn build_cpvm_configs_from_shard(
    path: &str,
    cref_default: f64,
    lambda_clf: f64,
    mu_cbf: f64,
) -> Result<Vec<CpvmNodeConfig>, CpvmLinkerError> {
    let metas = load_cpvm_nodes_from_csv(path)?;
    let configs = metas
        .into_iter()
        .map(|m| bind_cpvm_config(m, cref_default, lambda_clf, mu_cbf))
        .collect();
    Ok(configs)
}

/// Optional: small smoke test demonstrating loading and evaluation.
///
/// This is intentionally simple and can be moved into a proper test harness
/// in a larger workspace.
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_split_csv_line_basic() {
        let line = "A,B,C";
        let f = split_csv_line(line);
        assert_eq!(f, vec!["A", "B", "C"]);
    }

    #[test]
    fn test_split_csv_line_quotes() {
        let line = "A,\"B, with comma\",C";
        let f = split_csv_line(line);
        assert_eq!(f, vec!["A", "B, with comma", "C"]);
    }

    #[test]
    fn test_compute_mass_avoided() {
        let m = compute_mass_avoided(10.0, 5.0, 2.0, 100.0);
        assert!((m - 1000.0).abs() < 1e-6);
    }

    #[test]
    fn test_evaluate_ecoimpact_for_node() {
        let meta = CpvmNodeMeta {
            node_id: NodeId("TEST-NODE".to_string()),
            asset_type: AssetType::Plant,
            waterbody: "TestRiver".to_string(),
            region: "TestRegion".to_string(),
            cpvm_profile: "TEST_PROFILE".to_string(),
            cin_baseline: 10.0,
            cin_unit: ConcentrationUnit::MgPerL,
            q_avg: 1.0,
            q_unit: FlowUnit::M3PerS,
            horizon_s: 3600.0,
            ecoimpactscore: 0.8,
            karma_per_unit: 1.0e3,
            notes: "Test node".to_string(),
        };

        let cfg = bind_cpvm_config(meta, 5.0, 10.0, 100.0);
        let res = evaluate_ecoimpact_for_node(&cfg, 3.0);

        assert!(res.mass_avoided > 0.0);
        assert!(res.karma_gain > 0.0);
        assert!(res.ecoimpactscore <= 1.0);
    }
}
