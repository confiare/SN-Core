use crate::errors::*;

use crate::{proto, base, Warnable};

use crate::components::{Component, Sensitivity};
use crate::base::{Value, NodeProperties, AggregatorProperties, SensitivitySpace, ValueProperties, DataType, IndexKey};
use crate::utilities::prepend;
use ndarray::prelude::*;
use indexmap::map::IndexMap;


impl Component for proto::Sum {
    fn propagate_property(
        &self,
        _privacy_definition: &Option<proto::PrivacyDefinition>,
        _public_arguments: &IndexMap<base::IndexKey, Value>,
        properties: &base::NodeProperties,
        node_id: u32
    ) -> Result<Warnable<ValueProperties>> {
        let mut data_property = properties.get::<IndexKey>(&"data".into())
            .ok_or("data: missing")?.array()
            .map_err(prepend("data:"))?.clone();

        if !data_property.releasable {
            data_property.assert_is_not_aggregated()?;
        }

        // save a snapshot of the state when aggregating
        data_property.aggregator = Some(AggregatorProperties {
            component: proto::component::Variant::Sum(self.clone()),
            properties: properties.clone(),
            c_stability: data_property.c_stability.clone(),
            lipschitz_constant: (0..data_property.num_columns()?).map(|_| 1.).collect()
        });

        if data_property.data_type != DataType::F64 && data_property.data_type != DataType::I64 {
            return Err("data: atomic type must be numeric".into())
        }

        data_property.num_records = Some(1);
        data_property.nature = None;
        data_property.dataset_id = Some(node_id as i64);

        Ok(ValueProperties::Array(data_property).into())
    }
}

impl Sensitivity for proto::Sum {
    /// Sum sensitivities [are backed by the the proofs here](https://github.com/opendifferentialprivacy/whitenoise-core/blob/955703e3d80405d175c8f4642597ccdf2c00332a/whitepapers/sensitivities/sums/sums.pdf)
    fn compute_sensitivity(
        &self,
        privacy_definition: &proto::PrivacyDefinition,
        properties: &NodeProperties,
        sensitivity_type: &SensitivitySpace,
    ) -> Result<Value> {

        match sensitivity_type {

            SensitivitySpace::KNorm(k) => {

                let data_property = properties.get::<IndexKey>(&"data".into())
                    .ok_or("data: missing")?.array()
                    .map_err(prepend("data:"))?.clone();

                data_property.assert_is_not_aggregated()?;
                data_property.assert_non_null()?;
                let data_lower = data_property.lower_f64()?;
                let data_upper = data_property.upper_f64()?;

                use proto::privacy_definition::Neighboring;
                let neighboring_type = Neighboring::from_i32(privacy_definition.neighboring)
                    .ok_or_else(|| Error::from("neighboring definition must be either \"AddRemove\" or \"Substitute\""))?;

                let row_sensitivity = match k {
                    1 | 2 => match neighboring_type {
                        Neighboring::AddRemove => data_lower.iter()
                            .zip(data_upper.iter())
                            .map(|(min, max)| min.abs().max(max.abs()))
                            .collect::<Vec<f64>>(),
                        Neighboring::Substitute => data_lower.iter()
                            .zip(data_upper.iter())
                            .map(|(min, max)| (max - min))
                            .collect::<Vec<f64>>()
                    }
                    _ => return Err("KNorm sensitivity is only supported in L1 and L2 spaces".into())
                };

                let mut array_sensitivity = Array::from(row_sensitivity).into_dyn();
                array_sensitivity.insert_axis_inplace(Axis(0));

                Ok(array_sensitivity.into())
            }
            _ => Err("Sum sensitivity is only implemented for KNorm of 1".into())
        }
    }
}